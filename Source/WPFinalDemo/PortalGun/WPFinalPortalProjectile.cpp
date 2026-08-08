// Copyright 2026 Team Beaver. All Rights Reserved.

#include "WPFinalPortalProjectile.h"

#include "WPFinalPortalBurstEffect.h"
#include "WPFinalPortalGunComponent.h"
#include "WPFinalPortalGunConstants.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace PortalConstants = WPFinalPortalGunConstants;

namespace
{
	void SetupProjectileMesh(UStaticMeshComponent* const Mesh, const int32 SortPriority)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(false);
		Mesh->SetReceivesDecals(false);
		Mesh->SetTranslucentSortPriority(SortPriority);
	}

	FVector MakeSphereScale(const float Length, const float Radius)
	{
		return FVector(FMath::Max(Length, 0.01f) / PortalConstants::Burst::SphereLength, FMath::Max(Radius, 0.01f) / PortalConstants::Burst::SphereDiameter,
			FMath::Max(Radius, 0.01f) / PortalConstants::Burst::SphereDiameter);
	}
} // namespace

AWPFinalPortalProjectile::AWPFinalPortalProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	InitialLifeSpan = PortalConstants::Projectile::LifeTime;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(PortalConstants::Projectile::CollisionRadius);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComponent->SetGenerateOverlapEvents(false);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Energy Bolt"));
	CoreMesh->SetupAttachment(CollisionComponent);
	SetupProjectileMesh(CoreMesh, PortalConstants::Projectile::CoreSortPriority);
	CoreMesh->SetRelativeLocation(FVector(-PortalConstants::Projectile::CoreBackOffset, 0.0f, 0.0f));
	CoreMesh->SetRelativeScale3D(MakeSphereScale(PortalConstants::Projectile::CoreLength, PortalConstants::Projectile::CoreRadius));

	BlurMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Energy Blur Halo"));
	BlurMesh->SetupAttachment(CollisionComponent);
	SetupProjectileMesh(BlurMesh, PortalConstants::Projectile::BlurSortPriority);
	BlurMesh->SetRelativeLocation(FVector(-PortalConstants::Projectile::BlurBackOffset, 0.0f, 0.0f));
	BlurMesh->SetRelativeScale3D(MakeSphereScale(PortalConstants::Projectile::BlurLength, PortalConstants::Projectile::BlurRadius));

	TrailSegmentMeshes.Reserve(PortalConstants::Trail::SegmentCount);
	for (int32 SegmentIndex = 0; SegmentIndex < PortalConstants::Trail::SegmentCount; ++SegmentIndex)
	{
		const FName ComponentName(*FString::Printf(TEXT("Energy Trail %02d"), SegmentIndex));
		UStaticMeshComponent* const TrailSegment = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		TrailSegment->SetupAttachment(CollisionComponent);
		SetupProjectileMesh(TrailSegment, PortalConstants::Projectile::TrailSortPriority - SegmentIndex);
		TrailSegment->SetVisibility(false);

		const float SegmentAlpha =
			PortalConstants::Trail::SegmentCount > 1 ? static_cast<float>(SegmentIndex) / static_cast<float>(PortalConstants::Trail::SegmentCount - 1) : 0.0f;
		const float SegmentRadius = FMath::Lerp(PortalConstants::Trail::StartRadius, PortalConstants::Trail::EndRadius, SegmentAlpha);
		TrailSegment->SetRelativeLocation(FVector(-PortalConstants::Trail::Length * PortalConstants::Trail::CenterFractions[SegmentIndex], 0.0f, 0.0f));
		TrailSegment->SetRelativeScale3D(
			MakeSphereScale(PortalConstants::Trail::Length * PortalConstants::Trail::LengthFractions[SegmentIndex], SegmentRadius));
		TrailSegmentMeshes.Add(TrailSegment);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CoreMesh->SetStaticMesh(SphereMesh.Object);
		BlurMesh->SetStaticMesh(SphereMesh.Object);
		for (UStaticMeshComponent* const TrailSegment : TrailSegmentMeshes)
		{
			TrailSegment->SetStaticMesh(SphereMesh.Object);
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EnergyMaterialFinder(PortalConstants::Asset::EnergyMaterial);
	if (EnergyMaterialFinder.Succeeded())
	{
		CoreMesh->SetMaterial(0, EnergyMaterialFinder.Object);
		BlurMesh->SetMaterial(0, EnergyMaterialFinder.Object);
		for (UStaticMeshComponent* const TrailSegment : TrailSegmentMeshes)
		{
			TrailSegment->SetMaterial(0, EnergyMaterialFinder.Object);
		}
	}

	ProjectileLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Projectile Light"));
	ProjectileLight->SetupAttachment(CollisionComponent);
	ProjectileLight->SetIntensity(PortalConstants::Projectile::LightIntensity);
	ProjectileLight->SetAttenuationRadius(PortalConstants::Projectile::LightRadius);
	ProjectileLight->SetSourceRadius(PortalConstants::Projectile::LightSourceRadius);
	ProjectileLight->SetCastShadows(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovement->InitialSpeed = PortalConstants::Weapon::ProjectileSpeed;
	ProjectileMovement->MaxSpeed = PortalConstants::Weapon::ProjectileSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->MaxSimulationTimeStep = PortalConstants::Projectile::MaxSimulationStep;
	ProjectileMovement->MaxSimulationIterations = PortalConstants::Projectile::MaxSimulationIterations;
}

void AWPFinalPortalProjectile::InitializeProjectile(UWPFinalPortalGunComponent* const InPortalGun, const bool bBluePortal)
{
	PortalGunComponent = InPortalGun;
	bCreatesBluePortal = bBluePortal;

	const FLinearColor& PortalColor = PortalConstants::Color::Select(bBluePortal);
	const FLinearColor EmissiveColor(PortalColor.R * PortalConstants::Projectile::EmissiveColorMultiplier,
		PortalColor.G * PortalConstants::Projectile::EmissiveColorMultiplier, PortalColor.B * PortalConstants::Projectile::EmissiveColorMultiplier, 1.0f);

	const auto ApplyEnergyMaterial = [this, &PortalColor, &EmissiveColor](UStaticMeshComponent* const Mesh, const float EmissiveStrength, const float Opacity)
	{
		if (!Mesh)
		{
			return;
		}

		UMaterialInstanceDynamic* const DynamicMaterial = Mesh->CreateDynamicMaterialInstance(0);
		if (!DynamicMaterial)
		{
			return;
		}
		DynamicMaterial->SetVectorParameterValue(PortalConstants::Material::EffectColor, PortalColor);
		DynamicMaterial->SetVectorParameterValue(PortalConstants::Material::DiffuseColor, PortalColor);
		DynamicMaterial->SetVectorParameterValue(PortalConstants::Material::EmissiveColor, EmissiveColor);
		DynamicMaterial->SetScalarParameterValue(PortalConstants::Material::EmissiveStrength, EmissiveStrength);
		DynamicMaterial->SetScalarParameterValue(PortalConstants::Material::Opacity, Opacity);
		DynamicMaterial->SetScalarParameterValue(PortalConstants::Material::EmissiveColorMapWeight, 0.0f);
		Mesh->SetMaterial(0, DynamicMaterial);
	};

	ApplyEnergyMaterial(CoreMesh, PortalConstants::Projectile::CoreEmissiveStrength, PortalConstants::Projectile::CoreOpacity);
	ApplyEnergyMaterial(BlurMesh, PortalConstants::Projectile::BlurEmissiveStrength, PortalConstants::Projectile::BlurOpacity);

	for (int32 SegmentIndex = 0; SegmentIndex < TrailSegmentMeshes.Num(); ++SegmentIndex)
	{
		const float SegmentAlpha = TrailSegmentMeshes.Num() > 1 ? static_cast<float>(SegmentIndex) / static_cast<float>(TrailSegmentMeshes.Num() - 1) : 0.0f;
		ApplyEnergyMaterial(TrailSegmentMeshes[SegmentIndex],
			FMath::Lerp(PortalConstants::Trail::StartEmissiveStrength, PortalConstants::Trail::EndEmissiveStrength, SegmentAlpha),
			FMath::Lerp(PortalConstants::Trail::StartOpacity, PortalConstants::Trail::EndOpacity, SegmentAlpha));
	}

	ProjectileLight->SetLightColor(PortalColor);
	ProjectileMovement->Velocity = GetActorForwardVector() * PortalConstants::Weapon::ProjectileSpeed;
	ProjectileMovement->UpdateComponentVelocity();
}

void AWPFinalPortalProjectile::BeginPlay()
{
	Super::BeginPlay();

	SpawnOrigin = GetActorLocation();
	CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
}

void AWPFinalPortalProjectile::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float TravelDistance = FVector::Distance(SpawnOrigin, GetActorLocation());
	for (int32 SegmentIndex = 0; SegmentIndex < TrailSegmentMeshes.Num(); ++SegmentIndex)
	{
		TrailSegmentMeshes[SegmentIndex]->SetVisibility(
			TravelDistance >= PortalConstants::Trail::Length * PortalConstants::Trail::RevealFractions[SegmentIndex]);
	}
}

void AWPFinalPortalProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation,
	FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasImpacted)
	{
		return;
	}

	bHasImpacted = true;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->StopMovementImmediately();
	CoreMesh->SetVisibility(false);
	BlurMesh->SetVisibility(false);
	for (UStaticMeshComponent* const TrailSegment : TrailSegmentMeshes)
	{
		TrailSegment->SetVisibility(false);
	}
	ProjectileLight->SetVisibility(false);

	UWorld* const World = GetWorld();
	if (World)
	{
		FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
		if (ImpactNormal.IsNearlyZero())
		{
			ImpactNormal = HitNormal.GetSafeNormal();
		}
		if (ImpactNormal.IsNearlyZero())
		{
			ImpactNormal = FVector::ForwardVector;
		}

		const FVector ImpactPoint = Hit.ImpactPoint.IsNearlyZero() ? FVector(HitLocation) : FVector(Hit.ImpactPoint);
		const FTransform EffectTransform(
			ImpactNormal.Rotation(), ImpactPoint + ImpactNormal * PortalConstants::Projectile::ImpactEffectOffset, FVector::OneVector);

		FActorSpawnParameters EffectSpawnParameters;
		EffectSpawnParameters.Owner = GetOwner();
		EffectSpawnParameters.Instigator = GetInstigator();
		EffectSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AWPFinalPortalBurstEffect* const ImpactEffect =
			World->SpawnActor<AWPFinalPortalBurstEffect>(AWPFinalPortalBurstEffect::StaticClass(), EffectTransform, EffectSpawnParameters);
		if (ImpactEffect)
		{
			ImpactEffect->InitializeImpact(bCreatesBluePortal);
		}
	}

	if (UWPFinalPortalGunComponent* const PortalGun = PortalGunComponent.Get())
	{
		if (bCreatesBluePortal)
		{
			PortalGun->PlacePortalA(Hit);
		}
		else
		{
			PortalGun->PlacePortalB(Hit);
		}
	}

	Destroy();
}
