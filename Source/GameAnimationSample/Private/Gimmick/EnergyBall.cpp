// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/EnergyBall.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Transit/WPTransitComponent.h"
#include "Transit/WPTransitTags.h"
#include "Transit/WPTransitTypes.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 EnergyArcCount = 7;
	constexpr int32 TrailWispCount = 5;
	const FName ColorParameterName(TEXT("Color"));
	const FName TextureParameterName(TEXT("Texture"));

	void ConfigureEnergyMesh(UStaticMeshComponent* Mesh, const int32 SortPriority)
	{
		if (!IsValid(Mesh)) return;

		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(false);
		Mesh->SetReceivesDecals(false);
		Mesh->SetTranslucentSortPriority(SortPriority);
	}

	FLinearColor MakeEmissiveColor(const FLinearColor& Color, const float Strength)
	{
		FLinearColor Result = Color * FMath::Max(Strength, 0.0f);
		Result.A = 1.0f;
		return Result;
	}
}

AEnergyBall::AEnergyBall()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(20.0f);
	CollisionSphere->SetMobility(EComponentMobility::Movable);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->SetSimulatePhysics(false);

	EnergyVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EnergyVisualRoot"));
	EnergyVisualRoot->SetupAttachment(CollisionSphere);

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	CoreMesh->SetupAttachment(EnergyVisualRoot);
	CoreMesh->SetRelativeScale3D(FVector(0.18f));
	ConfigureEnergyMesh(CoreMesh, 4);

	PlasmaShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlasmaShellMesh"));
	PlasmaShellMesh->SetupAttachment(EnergyVisualRoot);
	PlasmaShellMesh->SetRelativeScale3D(FVector(0.34f));
	ConfigureEnergyMesh(PlasmaShellMesh, 3);

	HaloMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HaloMesh"));
	HaloMesh->SetupAttachment(EnergyVisualRoot);
	HaloMesh->SetRelativeScale3D(FVector(0.52f));
	ConfigureEnergyMesh(HaloMesh, 1);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CoreMesh->SetStaticMesh(SphereMesh.Object);
		PlasmaShellMesh->SetStaticMesh(SphereMesh.Object);
		HaloMesh->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	for (int32 ArcIndex = 0; ArcIndex < EnergyArcCount; ++ArcIndex)
	{
		const FName ArcName(*FString::Printf(TEXT("EnergyArc_%02d"), ArcIndex));
		UStaticMeshComponent* ArcMesh = CreateDefaultSubobject<UStaticMeshComponent>(ArcName);
		ArcMesh->SetupAttachment(EnergyVisualRoot);
		ArcMesh->SetRelativeScale3D(FVector(0.012f, 0.012f, 0.20f));
		ConfigureEnergyMesh(ArcMesh, 5);
		if (CylinderMesh.Succeeded())
		{
			ArcMesh->SetStaticMesh(CylinderMesh.Object);
		}
		EnergyArcs.Add(ArcMesh);
	}

	for (int32 WispIndex = 0; WispIndex < TrailWispCount; ++WispIndex)
	{
		const FName WispName(*FString::Printf(TEXT("TrailWisp_%02d"), WispIndex));
		UStaticMeshComponent* WispMesh = CreateDefaultSubobject<UStaticMeshComponent>(WispName);
		WispMesh->SetupAttachment(CollisionSphere);
		ConfigureEnergyMesh(WispMesh, 2);
		if (SphereMesh.Succeeded())
		{
			WispMesh->SetStaticMesh(SphereMesh.Object);
		}
		TrailWisps.Add(WispMesh);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AdditiveMaterial(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (AdditiveMaterial.Succeeded())
	{
		EmissiveMaterial = AdditiveMaterial.Object;
		CoreMesh->SetMaterial(0, EmissiveMaterial);
		PlasmaShellMesh->SetMaterial(0, EmissiveMaterial);
		HaloMesh->SetMaterial(0, EmissiveMaterial);
		for (UStaticMeshComponent* ArcMesh : EnergyArcs) ArcMesh->SetMaterial(0, EmissiveMaterial);
		for (UStaticMeshComponent* WispMesh : TrailWisps) WispMesh->SetMaterial(0, EmissiveMaterial);
	}

	static ConstructorHelpers::FObjectFinder<UTexture> WhiteTexture(
		TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	if (WhiteTexture.Succeeded())
	{
		EmissiveTexture = WhiteTexture.Object;
	}

	BallLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BallLight"));
	BallLight->SetupAttachment(CollisionSphere);
	BallLight->SetLightColor(EnergyColor);
	BallLight->SetIntensity(LightIntensity);
	BallLight->SetAttenuationRadius(350.0f);
	BallLight->SetCastShadows(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = bBounce;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->SetAutoActivate(false);

	TransitComponent = CreateDefaultSubobject<UWPTransitComponent>(TEXT("TransitComponent"));
	TransitComponent->SetTransitType(EWPTransitType::Projectile);
}

void AEnergyBall::BeginPlay()
{
	Super::BeginPlay();

	ApplyMovementSettings();
	InitializeVisualMaterials();
	UpdateEnergyVisuals(0.0f);
	ProjectileMovement->OnProjectileStop.AddUniqueDynamic(this, &AEnergyBall::HandleProjectileStop);

	if (MaxLifeSeconds > 0.0f)
	{
		SetLifeSpan(MaxLifeSeconds);
	}
}

void AEnergyBall::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateEnergyVisuals(DeltaSeconds);
}

void AEnergyBall::Launch(const FVector& WorldDirection, const float OverrideSpeed)
{
	if (!HasAuthority() || bConsumed || !IsValid(ProjectileMovement)) return;

	const FVector Direction = WorldDirection.GetSafeNormal();
	if (Direction.IsNearlyZero()) return;

	ApplyMovementSettings();
	const float LaunchSpeed = OverrideSpeed > 0.0f ? OverrideSpeed : Speed;
	ProjectileMovement->MaxSpeed = FMath::Max(ProjectileMovement->MaxSpeed, LaunchSpeed);
	ProjectileMovement->Velocity = Direction * LaunchSpeed;
	ProjectileMovement->Activate(true);
	ProjectileMovement->UpdateComponentVelocity();
	SetActorRotation(Direction.Rotation());
	ForceNetUpdate();
}

void AEnergyBall::Consume(AActor* ConsumedBy)
{
	if (!HasAuthority() || bConsumed || ActorHasTag(WPTransitTags::Generated)) return;

	bConsumed = true;
	OnConsumed(ConsumedBy);

	if (IsValid(TransitComponent))
	{
		TransitComponent->CancelTransit();
	}

	Destroy();
}

void AEnergyBall::ApplyMovementSettings()
{
	if (!IsValid(ProjectileMovement)) return;

	ProjectileMovement->MaxSpeed = FMath::Max(Speed, 1.0f);
	ProjectileMovement->ProjectileGravityScale = FMath::Max(GravityScale, 0.0f);
	ProjectileMovement->bShouldBounce = bBounce;
	ProjectileMovement->Bounciness = FMath::Clamp(Bounciness, 0.0f, 1.0f);
}

void AEnergyBall::InitializeVisualMaterials()
{
	if (!IsValid(EmissiveMaterial)) return;

	auto CreateMaterial = [this](UStaticMeshComponent* Mesh) -> UMaterialInstanceDynamic*
	{
		if (!IsValid(Mesh)) return nullptr;

		UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0);
		if (IsValid(Material) && IsValid(EmissiveTexture))
		{
			Material->SetTextureParameterValue(TextureParameterName, EmissiveTexture);
		}
		return Material;
	};

	CoreMaterial = CreateMaterial(CoreMesh);
	PlasmaMaterial = CreateMaterial(PlasmaShellMesh);
	HaloMaterial = CreateMaterial(HaloMesh);

	ArcMaterials.Reset(EnergyArcs.Num());
	for (UStaticMeshComponent* ArcMesh : EnergyArcs)
	{
		ArcMaterials.Add(CreateMaterial(ArcMesh));
	}

	TrailMaterials.Reset(TrailWisps.Num());
	for (UStaticMeshComponent* WispMesh : TrailWisps)
	{
		TrailMaterials.Add(CreateMaterial(WispMesh));
	}
}

void AEnergyBall::UpdateEnergyVisuals(const float DeltaSeconds)
{
	VisualTime += FMath::Max(DeltaSeconds, 0.0f);

	const float PulsePhase = VisualTime * FMath::Max(PulseSpeed, 0.0f);
	const float Pulse = 0.5f + 0.5f * FMath::Sin(PulsePhase);
	const float SecondaryPulse = 0.5f + 0.5f * FMath::Sin(PulsePhase * 1.73f + 1.2f);

	if (IsValid(EnergyVisualRoot))
	{
		EnergyVisualRoot->SetRelativeRotation(FRotator(
			VisualTime * 31.0f,
			VisualTime * -47.0f,
			VisualTime * 23.0f));
	}

	if (IsValid(CoreMesh))
	{
		CoreMesh->SetRelativeScale3D(FVector(0.175f + Pulse * 0.018f));
	}
	if (IsValid(PlasmaShellMesh))
	{
		PlasmaShellMesh->SetRelativeScale3D(FVector(0.32f + SecondaryPulse * 0.055f));
	}
	if (IsValid(HaloMesh))
	{
		HaloMesh->SetRelativeScale3D(FVector(0.48f + Pulse * 0.09f));
	}

	if (IsValid(CoreMaterial))
	{
		CoreMaterial->SetVectorParameterValue(
			ColorParameterName,
			MakeEmissiveColor(CoreColor, CoreEmissiveStrength * (0.9f + Pulse * 0.2f)));
	}
	if (IsValid(PlasmaMaterial))
	{
		PlasmaMaterial->SetVectorParameterValue(
			ColorParameterName,
			MakeEmissiveColor(EnergyColor, PlasmaEmissiveStrength * (0.75f + SecondaryPulse * 0.5f)));
	}
	if (IsValid(HaloMaterial))
	{
		HaloMaterial->SetVectorParameterValue(
			ColorParameterName,
			MakeEmissiveColor(EnergyColor, HaloEmissiveStrength * (0.7f + Pulse * 0.35f)));
	}

	for (int32 ArcIndex = 0; ArcIndex < EnergyArcs.Num(); ++ArcIndex)
	{
		UStaticMeshComponent* ArcMesh = EnergyArcs[ArcIndex];
		if (!IsValid(ArcMesh)) continue;

		const float ArcSeed = static_cast<float>(ArcIndex) * 0.897f;
		const float AngleDegrees = VisualTime * ArcRotationSpeed * (1.0f + ArcIndex * 0.035f)
			+ ArcIndex * (360.0f / FMath::Max(EnergyArcs.Num(), 1));
		const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
		const float Radius = 20.0f + FMath::Sin(PulsePhase * 1.9f + ArcSeed) * 4.5f;
		const float ForwardJitter = FMath::Sin(PulsePhase * 2.4f + ArcSeed * 3.0f) * 6.0f;
		const FVector ArcLocation(
			ForwardJitter,
			FMath::Cos(AngleRadians) * Radius,
			FMath::Sin(AngleRadians) * Radius);
		const FVector OrbitTangent(
			0.2f * FMath::Sin(PulsePhase + ArcSeed),
			-FMath::Sin(AngleRadians),
			FMath::Cos(AngleRadians));

		ArcMesh->SetRelativeLocation(ArcLocation);
		ArcMesh->SetRelativeRotation(FRotationMatrix::MakeFromZ(OrbitTangent.GetSafeNormal()).Rotator());
		const float ArcLength = 0.13f + 0.10f * (0.5f + 0.5f * FMath::Sin(PulsePhase * 2.7f + ArcSeed));
		ArcMesh->SetRelativeScale3D(FVector(0.010f, 0.010f, ArcLength));

		if (ArcMaterials.IsValidIndex(ArcIndex) && IsValid(ArcMaterials[ArcIndex]))
		{
			const float ArcFlicker = 1.6f + 2.8f * FMath::Abs(FMath::Sin(PulsePhase * 2.15f + ArcSeed));
			ArcMaterials[ArcIndex]->SetVectorParameterValue(
				ColorParameterName,
				MakeEmissiveColor(EnergyColor, ArcFlicker));
		}
	}

	for (int32 WispIndex = 0; WispIndex < TrailWisps.Num(); ++WispIndex)
	{
		UStaticMeshComponent* WispMesh = TrailWisps[WispIndex];
		if (!IsValid(WispMesh)) continue;

		const float IndexAlpha = static_cast<float>(WispIndex) / FMath::Max(TrailWisps.Num() - 1, 1);
		const float WispPhase = PulsePhase * (1.15f + WispIndex * 0.07f) + WispIndex * 1.37f;
		const float DistanceBehind = 26.0f + WispIndex * 18.0f;
		const float LateralRadius = 2.0f + WispIndex * 0.9f;
		WispMesh->SetRelativeLocation(FVector(
			-DistanceBehind,
			FMath::Sin(WispPhase) * LateralRadius,
			FMath::Cos(WispPhase * 1.31f) * LateralRadius));

		const float WispPulse = 0.75f + 0.25f * FMath::Sin(WispPhase * 1.7f);
		WispMesh->SetRelativeScale3D(FVector(
			FMath::Lerp(0.30f, 0.10f, IndexAlpha) * WispPulse,
			FMath::Lerp(0.095f, 0.025f, IndexAlpha),
			FMath::Lerp(0.095f, 0.025f, IndexAlpha)));

		if (TrailMaterials.IsValidIndex(WispIndex) && IsValid(TrailMaterials[WispIndex]))
		{
			const float WispStrength = FMath::Lerp(2.8f, 0.28f, IndexAlpha);
			TrailMaterials[WispIndex]->SetVectorParameterValue(
				ColorParameterName,
				MakeEmissiveColor(EnergyColor, WispStrength));
		}
	}

	if (IsValid(BallLight))
	{
		BallLight->SetLightColor(EnergyColor);
		BallLight->SetIntensity(FMath::Max(LightIntensity, 0.0f) * (0.82f + Pulse * 0.24f));
	}
}

void AEnergyBall::HandleProjectileStop(const FHitResult& Hit)
{
	if (!HasAuthority() || bConsumed || !bDestroyOnImpact || ActorHasTag(WPTransitTags::Generated)) return;

	bConsumed = true;
	OnImpacted(Hit);

	if (IsValid(TransitComponent))
	{
		TransitComponent->CancelTransit();
	}

	Destroy();
}
