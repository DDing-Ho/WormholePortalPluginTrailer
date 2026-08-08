// Copyright 2026 Team Beaver. All Rights Reserved.

#include "WPFinalPortalBurstEffect.h"

#include "WPFinalPortalGunConstants.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace PortalConstants = WPFinalPortalGunConstants;

namespace
{
	void SetupEnergyMesh(UStaticMeshComponent* const Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(false);
		Mesh->SetReceivesDecals(false);
		Mesh->SetVisibility(false);
	}

	FVector MakeSphereScale(const float Length, const float Radius)
	{
		return FVector(FMath::Max(Length, 0.01f) / PortalConstants::Burst::SphereLength, FMath::Max(Radius, 0.01f) / PortalConstants::Burst::SphereDiameter,
			FMath::Max(Radius, 0.01f) / PortalConstants::Burst::SphereDiameter);
	}

	float SmoothStep(const float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}
} // namespace

AWPFinalPortalBurstEffect::AWPFinalPortalBurstEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	InitialLifeSpan = 0.0f;
	SetActorEnableCollision(false);

	USceneComponent* const Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	OuterFlashMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Energy Flash"));
	OuterFlashMesh->SetupAttachment(Root);
	SetupEnergyMesh(OuterFlashMesh);
	OuterFlashMesh->SetTranslucentSortPriority(PortalConstants::Burst::OuterSortPriority);

	InnerCoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Energy Wave"));
	InnerCoreMesh->SetupAttachment(Root);
	SetupEnergyMesh(InnerCoreMesh);
	InnerCoreMesh->SetTranslucentSortPriority(PortalConstants::Burst::InnerSortPriority);

	ParticleMeshes.Reserve(PortalConstants::Muzzle::ParticlePoolSize);
	for (int32 ParticleIndex = 0; ParticleIndex < PortalConstants::Muzzle::ParticlePoolSize; ++ParticleIndex)
	{
		const FName ComponentName(*FString::Printf(TEXT("Energy Spark %02d"), ParticleIndex));
		UStaticMeshComponent* const ParticleMesh = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		ParticleMesh->SetupAttachment(Root);
		SetupEnergyMesh(ParticleMesh);
		ParticleMesh->SetTranslucentSortPriority(PortalConstants::Burst::ParticleSortPriority);
		ParticleMeshes.Add(ParticleMesh);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		OuterFlashMesh->SetStaticMesh(SphereMesh.Object);
		InnerCoreMesh->SetStaticMesh(SphereMesh.Object);
		for (UStaticMeshComponent* const ParticleMesh : ParticleMeshes)
		{
			ParticleMesh->SetStaticMesh(SphereMesh.Object);
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EnergyMaterialFinder(PortalConstants::Asset::EnergyMaterial);
	if (EnergyMaterialFinder.Succeeded())
	{
		OuterFlashMesh->SetMaterial(0, EnergyMaterialFinder.Object);
		InnerCoreMesh->SetMaterial(0, EnergyMaterialFinder.Object);
		for (UStaticMeshComponent* const ParticleMesh : ParticleMeshes)
		{
			ParticleMesh->SetMaterial(0, EnergyMaterialFinder.Object);
		}
	}

	EffectLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Burst Light"));
	EffectLight->SetupAttachment(Root);
	EffectLight->SetCastShadows(false);
	EffectLight->SetSourceRadius(PortalConstants::Burst::LightSourceRadius);
	EffectLight->SetIntensity(0.0f);
}

void AWPFinalPortalBurstEffect::InitializeMuzzle(const bool bBluePortal)
{
	InitializeEffect(bBluePortal, false);
}

void AWPFinalPortalBurstEffect::InitializeImpact(const bool bBluePortal)
{
	InitializeEffect(bBluePortal, true);
}

void AWPFinalPortalBurstEffect::InitializeEffect(const bool bBluePortal, const bool bImpact)
{
	bIsImpact = bImpact;
	ElapsedSeconds = 0.0f;

	const float LifeTime = bIsImpact ? PortalConstants::Impact::LifeTime : PortalConstants::Muzzle::LifeTime;
	const float OuterOpacity = bIsImpact ? PortalConstants::Impact::FlashOpacity : PortalConstants::Muzzle::OuterOpacity;
	const float InnerOpacity = bIsImpact ? PortalConstants::Impact::WaveOpacity : PortalConstants::Muzzle::WhiteCoreOpacity;
	const float ParticleOpacity = bIsImpact ? PortalConstants::Impact::ParticleOpacity : PortalConstants::Muzzle::ParticleOpacity;
	SetLifeSpan(LifeTime + PortalConstants::Burst::DestroyDelay);

	const FLinearColor& PortalColor = PortalConstants::Color::Select(bBluePortal);
	const FLinearColor InnerColor = bIsImpact ? PortalColor : FMath::Lerp(PortalColor, FLinearColor::White, PortalConstants::Muzzle::WhiteCoreBlend);
	const FLinearColor ParticleColor = bIsImpact ? PortalColor : FMath::Lerp(PortalColor, FLinearColor::White, PortalConstants::Muzzle::ParticleWhiteBlend);

	OuterFlashMaterial = OuterFlashMesh->CreateDynamicMaterialInstance(0);
	InnerCoreMaterial = InnerCoreMesh->CreateDynamicMaterialInstance(0);
	if (!ParticleMeshes.IsEmpty() && ParticleMeshes[0]->GetMaterial(0))
	{
		ParticleMaterial = UMaterialInstanceDynamic::Create(ParticleMeshes[0]->GetMaterial(0), this);
	}

	if (OuterFlashMaterial)
	{
		OuterFlashMaterial->SetVectorParameterValue(PortalConstants::Material::EffectColor, PortalColor);
		OuterFlashMaterial->SetScalarParameterValue(PortalConstants::Material::EmissiveStrength,
			bIsImpact ? PortalConstants::Impact::FlashEmissiveStrength : PortalConstants::Muzzle::OuterEmissiveStrength);
		OuterFlashMaterial->SetScalarParameterValue(PortalConstants::Material::Opacity, OuterOpacity);
	}

	if (InnerCoreMaterial)
	{
		InnerCoreMaterial->SetVectorParameterValue(PortalConstants::Material::EffectColor, InnerColor);
		InnerCoreMaterial->SetScalarParameterValue(PortalConstants::Material::EmissiveStrength,
			bIsImpact ? PortalConstants::Impact::WaveEmissiveStrength : PortalConstants::Muzzle::WhiteCoreEmissiveStrength);
		InnerCoreMaterial->SetScalarParameterValue(PortalConstants::Material::Opacity, InnerOpacity);
	}

	if (ParticleMaterial)
	{
		ParticleMaterial->SetVectorParameterValue(PortalConstants::Material::EffectColor, ParticleColor);
		ParticleMaterial->SetScalarParameterValue(PortalConstants::Material::EmissiveStrength,
			bIsImpact ? PortalConstants::Impact::ParticleEmissiveStrength : PortalConstants::Muzzle::ParticleEmissiveStrength);
		ParticleMaterial->SetScalarParameterValue(PortalConstants::Material::Opacity, ParticleOpacity);
		for (UStaticMeshComponent* const ParticleMesh : ParticleMeshes)
		{
			ParticleMesh->SetMaterial(0, ParticleMaterial);
		}
	}

	const FVector OuterScale = bIsImpact ? MakeSphereScale(PortalConstants::Impact::FlashLength, PortalConstants::Impact::FlashRadius)
										 : MakeSphereScale(PortalConstants::Muzzle::OuterLength, PortalConstants::Muzzle::OuterRadius);
	const FVector InnerScale = bIsImpact ? MakeSphereScale(PortalConstants::Impact::WaveLength, PortalConstants::Impact::WaveRadius)
										 : MakeSphereScale(PortalConstants::Muzzle::WhiteCoreLength, PortalConstants::Muzzle::WhiteCoreRadius);

	OuterFlashMesh->SetRelativeLocation(FVector(bIsImpact ? PortalConstants::Impact::FlashForwardOffset : PortalConstants::Muzzle::OuterCenter, 0.0f, 0.0f));
	InnerCoreMesh->SetRelativeLocation(FVector(bIsImpact ? PortalConstants::Impact::WaveForwardOffset : PortalConstants::Muzzle::WhiteCoreCenter, 0.0f,
		bIsImpact ? 0.0f : PortalConstants::Muzzle::WhiteCoreHeight));
	OuterFlashMesh->SetRelativeScale3D(OuterScale);
	InnerCoreMesh->SetRelativeScale3D(InnerScale);
	OuterFlashMesh->SetVisibility(true);
	InnerCoreMesh->SetVisibility(true);

	FRandomStream RandomStream(GetUniqueID() * PortalConstants::Burst::RandomSeedMultiplier +
							   (bBluePortal ? PortalConstants::Burst::BlueRandomSeed : PortalConstants::Burst::OrangeRandomSeed));
	CreateParticles(RandomStream);

	EffectLight->SetLightColor(PortalColor);
	EffectLight->SetAttenuationRadius(bIsImpact ? PortalConstants::Impact::LightRadius : PortalConstants::Muzzle::LightRadius);
	EffectLight->SetIntensity(bIsImpact ? PortalConstants::Impact::LightIntensity : PortalConstants::Muzzle::LightIntensity);
	SetActorTickEnabled(true);
}

void AWPFinalPortalBurstEffect::CreateParticles(FRandomStream& RandomStream)
{
	ParticlePositions.Reset(PortalConstants::Muzzle::ParticlePoolSize);
	ParticleVelocities.Reset(PortalConstants::Muzzle::ParticlePoolSize);
	ParticleBaseScales.Reset(PortalConstants::Muzzle::ParticlePoolSize);

	const int32 VisibleParticleCount = bIsImpact ? PortalConstants::Impact::VisibleParticleCount : PortalConstants::Muzzle::VisibleParticleCount;
	const float Spread = bIsImpact ? 1.0f : PortalConstants::Muzzle::ParticleSpread;
	const float MinSpeed = bIsImpact ? PortalConstants::Impact::ParticleMinSpeed : PortalConstants::Muzzle::ParticleMinSpeed;
	const float MaxSpeed = bIsImpact ? PortalConstants::Impact::ParticleMaxSpeed : PortalConstants::Muzzle::ParticleMaxSpeed;
	const float MinLength = bIsImpact ? PortalConstants::Impact::ParticleMinLength : PortalConstants::Muzzle::ParticleMinLength;
	const float MaxLength = bIsImpact ? PortalConstants::Impact::ParticleMaxLength : PortalConstants::Muzzle::ParticleMaxLength;
	const float MinRadius = bIsImpact ? PortalConstants::Impact::ParticleMinRadius : PortalConstants::Muzzle::ParticleMinRadius;
	const float MaxRadius = bIsImpact ? PortalConstants::Impact::ParticleMaxRadius : PortalConstants::Muzzle::ParticleMaxRadius;

	for (int32 ParticleIndex = 0; ParticleIndex < ParticleMeshes.Num(); ++ParticleIndex)
	{
		UStaticMeshComponent* const ParticleMesh = ParticleMeshes[ParticleIndex];
		ParticleMesh->SetVisibility(ParticleIndex < VisibleParticleCount);

		FVector Direction(1.0f, RandomStream.FRandRange(-Spread, Spread), RandomStream.FRandRange(-Spread, Spread));
		Direction.Normalize();
		const FVector Velocity = Direction * RandomStream.FRandRange(MinSpeed, MaxSpeed);

		FVector Position(PortalConstants::Burst::ImpactParticleStart, 0.0f, 0.0f);
		if (!bIsImpact)
		{
			const float SpawnAngle = RandomStream.FRandRange(0.0f, UE_TWO_PI);
			const float SpawnRadius = FMath::Sqrt(RandomStream.FRand()) * PortalConstants::Muzzle::ParticleSpawnRadius;
			Position = FVector(PortalConstants::Muzzle::ParticleSpawnForwardOffset, FMath::Cos(SpawnAngle) * SpawnRadius, FMath::Sin(SpawnAngle) * SpawnRadius);
		}

		const FVector Scale = MakeSphereScale(RandomStream.FRandRange(MinLength, MaxLength), RandomStream.FRandRange(MinRadius, MaxRadius));
		ParticlePositions.Add(Position);
		ParticleVelocities.Add(Velocity);
		ParticleBaseScales.Add(Scale);
		ParticleMesh->SetRelativeLocation(Position);
		ParticleMesh->SetRelativeRotation(Direction.Rotation());
		ParticleMesh->SetRelativeScale3D(Scale);
	}
}

void AWPFinalPortalBurstEffect::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedSeconds += FMath::Max(DeltaSeconds, 0.0f);
	const float LifeTime = bIsImpact ? PortalConstants::Impact::LifeTime : PortalConstants::Muzzle::LifeTime;
	const float NormalizedAge = FMath::Clamp(ElapsedSeconds / LifeTime, 0.0f, 1.0f);
	const float EaseOut = 1.0f - FMath::Pow(1.0f - NormalizedAge, PortalConstants::Burst::EaseOutPower);
	const float FadeAlpha = SmoothStep((NormalizedAge - PortalConstants::Burst::FadeStart) / (1.0f - PortalConstants::Burst::FadeStart));
	float Opacity = 1.0f - FadeAlpha;

	const FVector OuterBaseScale = bIsImpact ? MakeSphereScale(PortalConstants::Impact::FlashLength, PortalConstants::Impact::FlashRadius)
											 : MakeSphereScale(PortalConstants::Muzzle::OuterLength, PortalConstants::Muzzle::OuterRadius);
	const FVector InnerBaseScale = bIsImpact ? MakeSphereScale(PortalConstants::Impact::WaveLength, PortalConstants::Impact::WaveRadius)
											 : MakeSphereScale(PortalConstants::Muzzle::WhiteCoreLength, PortalConstants::Muzzle::WhiteCoreRadius);

	if (bIsImpact)
	{
		OuterFlashMesh->SetRelativeScale3D(OuterBaseScale * FMath::Lerp(1.0f, PortalConstants::Impact::FlashExpansion, EaseOut));
		InnerCoreMesh->SetRelativeScale3D(FVector(InnerBaseScale.X * FMath::Lerp(1.0f, PortalConstants::Burst::ImpactWaveEndLengthRatio, NormalizedAge),
			InnerBaseScale.Y * FMath::Lerp(1.0f, PortalConstants::Impact::WaveExpansion, EaseOut),
			InnerBaseScale.Z * FMath::Lerp(1.0f, PortalConstants::Impact::WaveExpansion, EaseOut)));
	}
	else
	{
		const float AttackAlpha = SmoothStep(ElapsedSeconds / PortalConstants::Muzzle::AttackTime);
		const float CollapseAlpha = SmoothStep((NormalizedAge - PortalConstants::Muzzle::CollapseStart) / (1.0f - PortalConstants::Muzzle::CollapseStart));
		Opacity = AttackAlpha * (1.0f - CollapseAlpha);

		OuterFlashMesh->SetRelativeScale3D(
			FVector(OuterBaseScale.X * FMath::Lerp(PortalConstants::Burst::MuzzleOuterStartLength, PortalConstants::Burst::MuzzleOuterPeakLength, AttackAlpha) *
						FMath::Lerp(1.0f, PortalConstants::Burst::MuzzleOuterEndLength, CollapseAlpha),
				OuterBaseScale.Y * FMath::Lerp(PortalConstants::Burst::MuzzleOuterStartRadius, 1.0f, AttackAlpha) *
					FMath::Lerp(1.0f, PortalConstants::Burst::MuzzleOuterEndRadius, CollapseAlpha),
				OuterBaseScale.Z * FMath::Lerp(PortalConstants::Burst::MuzzleOuterStartRadius, 1.0f, AttackAlpha) *
					FMath::Lerp(1.0f, PortalConstants::Burst::MuzzleOuterEndRadius, CollapseAlpha)));
		InnerCoreMesh->SetRelativeScale3D(
			InnerBaseScale * FMath::Lerp(PortalConstants::Burst::MuzzleInnerStartScale, PortalConstants::Burst::MuzzleInnerEndScale, EaseOut));
	}

	const float Gravity = bIsImpact ? PortalConstants::Impact::ParticleGravity : PortalConstants::Muzzle::ParticleGravity;
	const float Drag = bIsImpact ? PortalConstants::Impact::ParticleDrag : PortalConstants::Muzzle::ParticleDrag;
	const FVector WorldGravity(0.0f, 0.0f, -Gravity);
	const FVector LocalGravity = GetActorTransform().InverseTransformVectorNoScale(WorldGravity);
	const float DragMultiplier = FMath::Exp(-Drag * DeltaSeconds);

	for (int32 ParticleIndex = 0; ParticleIndex < ParticleMeshes.Num(); ++ParticleIndex)
	{
		ParticleVelocities[ParticleIndex] += LocalGravity * DeltaSeconds;
		ParticlePositions[ParticleIndex] += ParticleVelocities[ParticleIndex] * DeltaSeconds;
		ParticleVelocities[ParticleIndex] *= DragMultiplier;

		UStaticMeshComponent* const ParticleMesh = ParticleMeshes[ParticleIndex];
		ParticleMesh->SetRelativeLocation(ParticlePositions[ParticleIndex]);
		ParticleMesh->SetRelativeRotation(ParticleVelocities[ParticleIndex].Rotation());
		const FVector BaseScale = ParticleBaseScales[ParticleIndex];
		ParticleMesh->SetRelativeScale3D(FVector(BaseScale.X * FMath::Lerp(1.0f, PortalConstants::Burst::ParticleEndLengthRatio, NormalizedAge),
			BaseScale.Y * FMath::Max(Opacity, PortalConstants::Burst::ParticleMinimumRadiusRatio),
			BaseScale.Z * FMath::Max(Opacity, PortalConstants::Burst::ParticleMinimumRadiusRatio)));
	}

	UpdateOpacity(Opacity);
	const float InitialLightIntensity = bIsImpact ? PortalConstants::Impact::LightIntensity : PortalConstants::Muzzle::LightIntensity;
	EffectLight->SetIntensity(InitialLightIntensity * FMath::Square(1.0f - NormalizedAge));

	if (NormalizedAge >= 1.0f)
	{
		Destroy();
	}
}

void AWPFinalPortalBurstEffect::UpdateOpacity(const float Opacity)
{
	const float OuterOpacity = bIsImpact ? PortalConstants::Impact::FlashOpacity : PortalConstants::Muzzle::OuterOpacity;
	const float InnerOpacity = bIsImpact ? PortalConstants::Impact::WaveOpacity : PortalConstants::Muzzle::WhiteCoreOpacity;
	const float ParticleOpacity = bIsImpact ? PortalConstants::Impact::ParticleOpacity : PortalConstants::Muzzle::ParticleOpacity;

	if (OuterFlashMaterial)
	{
		OuterFlashMaterial->SetScalarParameterValue(PortalConstants::Material::Opacity, OuterOpacity * Opacity);
	}
	if (InnerCoreMaterial)
	{
		InnerCoreMaterial->SetScalarParameterValue(PortalConstants::Material::Opacity, InnerOpacity * Opacity);
	}
	if (ParticleMaterial)
	{
		ParticleMaterial->SetScalarParameterValue(PortalConstants::Material::Opacity, ParticleOpacity * Opacity);
	}
}
