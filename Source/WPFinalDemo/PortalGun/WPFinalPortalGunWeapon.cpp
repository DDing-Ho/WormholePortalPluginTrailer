// Copyright 2026 Team Beaver. All Rights Reserved.

#include "WPFinalPortalGunWeapon.h"

#include "WPFinalPortalBurstEffect.h"
#include "WPFinalPortalGunComponent.h"
#include "WPFinalPortalGunConstants.h"
#include "WPFinalPortalProjectile.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Variant_Shooter/Weapons/ShooterWeaponHolder.h"

namespace PortalConstants = WPFinalPortalGunConstants;

AWPFinalPortalGunWeapon::AWPFinalPortalGunWeapon()
{
	PortalGunComponent = CreateDefaultSubobject<UWPFinalPortalGunComponent>(TEXT("Portal Gun"));
	FirstPersonSmoothingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("First Person Smoothing Pivot"));
	FirstPersonSmoothingPivot->SetupAttachment(GetRootComponent());
	FirstPersonSmoothingTarget = CreateDefaultSubobject<USceneComponent>(TEXT("First Person Smoothing Target"));
	FirstPersonSmoothingTarget->SetupAttachment(GetRootComponent());

	ProjectileMuzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Projectile Muzzle"));
	ProjectileMuzzle->SetupAttachment(GetFirstPersonMesh());
	ProjectileMuzzle->SetRelativeLocation(PortalConstants::Weapon::MuzzleLocation);
	ProjectileMuzzle->SetRelativeRotation(PortalConstants::Weapon::MuzzleRotation);
	ProjectileMuzzle->SetAbsolute(false, false, true);
	ProjectileMuzzle->SetRelativeScale3D(FVector::OneVector);
	ProjectileMuzzle->ArrowColor = PortalConstants::Weapon::MuzzleArrowColor;
	ProjectileMuzzle->ArrowSize = PortalConstants::Weapon::MuzzleArrowSize;
	ProjectileMuzzle->SetHiddenInGame(true);

	MagazineSize = PortalConstants::Weapon::MagazineSize;
	RefireRate = PortalConstants::Weapon::RefireDelay;
	LastOrangeShotTime = -PortalConstants::Weapon::RefireDelay;
	bFullAuto = false;
	FiringRecoil = 0.0f;
	MuzzleOffset = 0.0f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GunMeshFinder(PortalConstants::Asset::GunMesh);
	if (GunMeshFinder.Succeeded())
	{
		GetFirstPersonMesh()->SetSkeletalMesh(GunMeshFinder.Object);
		GetThirdPersonMesh()->SetSkeletalMesh(GunMeshFinder.Object);
		ProjectileMuzzle->SetupAttachment(GetFirstPersonMesh(), PortalConstants::Weapon::MuzzleBone);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> GunAnimationFinder(PortalConstants::Asset::GunAnimationBlueprint);
	if (GunAnimationFinder.Succeeded())
	{
		GetFirstPersonMesh()->SetAnimInstanceClass(GunAnimationFinder.Class);
		GetThirdPersonMesh()->SetAnimInstanceClass(GunAnimationFinder.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireMontageFinder(PortalConstants::Asset::FireMontage);
	if (FireMontageFinder.Succeeded())
	{
		FireMontage = FireMontageFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TubeMaterialFinder(PortalConstants::Asset::GunTubeMaterial);
	if (TubeMaterialFinder.Succeeded())
	{
		TubeMaterial = TubeMaterialFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> FirstPersonCharacterAnimationFinder(PortalConstants::Asset::FirstPersonCharacterAnimation);
	if (FirstPersonCharacterAnimationFinder.Succeeded())
	{
		FirstPersonAnimInstanceClass = FirstPersonCharacterAnimationFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> ThirdPersonCharacterAnimationFinder(PortalConstants::Asset::ThirdPersonCharacterAnimation);
	if (ThirdPersonCharacterAnimationFinder.Succeeded())
	{
		ThirdPersonAnimInstanceClass = ThirdPersonCharacterAnimationFinder.Class;
	}

	GetFirstPersonMesh()->SetRelativeScale3D(FVector::OneVector);
	GetThirdPersonMesh()->SetRelativeScale3D(FVector::OneVector);
	FirstPersonMeshLocationOffset = PortalConstants::Weapon::DefaultFirstPersonLocation;
	FirstPersonMeshRotationOffset = PortalConstants::Weapon::DefaultFirstPersonRotation;
	bHideFirstPersonCharacterMesh = true;
}

void AWPFinalPortalGunWeapon::BeginPlay()
{
	Super::BeginPlay();

	ProjectileMuzzle->AttachToComponent(GetFirstPersonMesh(), FAttachmentTransformRules::KeepRelativeTransform, PortalConstants::Weapon::MuzzleBone);
	ProjectileMuzzle->SetRelativeLocation(PortalConstants::Weapon::MuzzleLocation);
	ProjectileMuzzle->SetRelativeRotation(PortalConstants::Weapon::MuzzleRotation);
	ProjectileMuzzle->SetAbsolute(false, false, true);
	ProjectileMuzzle->SetRelativeScale3D(FVector::OneVector);

	FirstPersonRestLocation = GetFirstPersonMesh()->GetRelativeLocation();
	ThirdPersonRestLocation = GetThirdPersonMesh()->GetRelativeLocation();

	InitializeFirstPersonSmoothing();
	CreateTubeMaterials();
}

void AWPFinalPortalGunWeapon::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFirstPersonSmoothing(DeltaSeconds);

	if (!bRecoilActive)
	{
		return;
	}

	RecoilElapsedSeconds += FMath::Max(DeltaSeconds, 0.0f);
	ApplyRecoil(CalculateRecoilAlpha());

	const float RecoilDuration = PortalConstants::Weapon::RecoilKickTime + PortalConstants::Weapon::RecoilRecoveryTime;
	if (RecoilElapsedSeconds >= RecoilDuration)
	{
		bRecoilActive = false;
		ApplyRecoil(0.0f);
	}
}

void AWPFinalPortalGunWeapon::FireProjectile(const FVector& TargetLocation)
{
	ShootPortal(TargetLocation, true);
}

void AWPFinalPortalGunWeapon::StartAlternateFiring()
{
	if (!WeaponOwner || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastOrangeShotTime < PortalConstants::Weapon::RefireDelay)
	{
		return;
	}

	LastOrangeShotTime = CurrentTime;
	ShootPortal(WeaponOwner->GetWeaponTargetLocation(), false);
}

void AWPFinalPortalGunWeapon::HandleUtilityAxisInput(const float AxisValue)
{
	if (PortalGunComponent)
	{
		PortalGunComponent->CyclePortalSpawnDirection(AxisValue);
	}
}

bool AWPFinalPortalGunWeapon::ShootPortal(const FVector& TargetLocation, const bool bBluePortal)
{
	UWorld* const World = GetWorld();
	if (!World || !ProjectileMuzzle || !PortalGunComponent)
	{
		return false;
	}

	const FVector MuzzleLocation = ProjectileMuzzle->GetComponentLocation();
	FVector AimDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = ProjectileMuzzle->GetForwardVector();
	}

	const FVector SpawnLocation = MuzzleLocation + AimDirection * PortalConstants::Weapon::ProjectileSpawnClearance;
	const FTransform SpawnTransform(AimDirection.Rotation(), SpawnLocation, FVector::OneVector);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = PawnOwner;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;

	AWPFinalPortalProjectile* const Projectile =
		World->SpawnActor<AWPFinalPortalProjectile>(AWPFinalPortalProjectile::StaticClass(), SpawnTransform, SpawnParameters);
	if (!Projectile)
	{
		return false;
	}

	Projectile->InitializeProjectile(PortalGunComponent, bBluePortal);
	SpawnMuzzleFlash(MuzzleLocation, AimDirection, bBluePortal);
	SetTubeColorForShot(bBluePortal);
	PlayFireAnimation();
	StartRecoil();
	return true;
}

void AWPFinalPortalGunWeapon::SpawnMuzzleFlash(const FVector& MuzzleLocation, const FVector& AimDirection, const bool bBluePortal)
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	const FTransform EffectTransform(AimDirection.Rotation(), MuzzleLocation + AimDirection * PortalConstants::Muzzle::SpawnForwardOffset, FVector::OneVector);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = PawnOwner;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWPFinalPortalBurstEffect* const MuzzleFlash =
		World->SpawnActor<AWPFinalPortalBurstEffect>(AWPFinalPortalBurstEffect::StaticClass(), EffectTransform, SpawnParameters);
	if (!MuzzleFlash)
	{
		return;
	}

	MuzzleFlash->AttachToComponent(ProjectileMuzzle, FAttachmentTransformRules::KeepWorldTransform);
	MuzzleFlash->InitializeMuzzle(bBluePortal);
}

void AWPFinalPortalGunWeapon::CreateTubeMaterials()
{
	if (!TubeMaterial)
	{
		return;
	}

	const auto CreateTubeMaterial = [this](USkeletalMeshComponent* const Mesh, const FName InstanceName)
	{
		if (!Mesh)
		{
			return static_cast<UMaterialInstanceDynamic*>(nullptr);
		}

		const FName MaterialSlotName(PortalConstants::Tube::MaterialSlot);
		const int32 MaterialIndex = Mesh->GetMaterialIndex(MaterialSlotName);
		if (MaterialIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Portal gun tube material slot '%s' was not found on %s."), *MaterialSlotName.ToString(), *GetNameSafe(Mesh));
			return static_cast<UMaterialInstanceDynamic*>(nullptr);
		}

		return Mesh->CreateDynamicMaterialInstance(MaterialIndex, TubeMaterial, InstanceName);
	};

	FirstPersonTubeDynamicMaterial = CreateTubeMaterial(GetFirstPersonMesh(), TEXT("PortalGunTube_FirstPerson"));
	ThirdPersonTubeDynamicMaterial = CreateTubeMaterial(GetThirdPersonMesh(), TEXT("PortalGunTube_ThirdPerson"));

	ApplyTubeMaterial(PortalConstants::Color::IdleTube, PortalConstants::Tube::IdleEmissiveStrength, PortalConstants::Tube::IdleOpacity);
}

void AWPFinalPortalGunWeapon::SetTubeColorForShot(const bool bBluePortal)
{
	if (!FirstPersonTubeDynamicMaterial && !ThirdPersonTubeDynamicMaterial)
	{
		CreateTubeMaterials();
	}

	ApplyTubeMaterial(PortalConstants::Color::Select(bBluePortal), PortalConstants::Tube::FiredEmissiveStrength, PortalConstants::Tube::FiredOpacity);
}

void AWPFinalPortalGunWeapon::ApplyTubeMaterial(const FLinearColor& Color, const float EmissiveStrength, const float Opacity)
{
	const auto Apply = [&Color, EmissiveStrength, Opacity](UMaterialInstanceDynamic* const Material)
	{
		if (!Material)
		{
			return;
		}

		Material->SetVectorParameterValue(PortalConstants::Material::EffectColor, Color);
		Material->SetScalarParameterValue(PortalConstants::Material::EmissiveStrength, EmissiveStrength);
		Material->SetScalarParameterValue(PortalConstants::Material::Opacity, Opacity);
	};

	Apply(FirstPersonTubeDynamicMaterial);
	Apply(ThirdPersonTubeDynamicMaterial);
}

void AWPFinalPortalGunWeapon::PlayFireAnimation()
{
	if (!FireMontage)
	{
		return;
	}

	if (UAnimInstance* const FirstPersonAnimation = GetFirstPersonMesh()->GetAnimInstance())
	{
		FirstPersonAnimation->Montage_Play(FireMontage);
	}
	if (UAnimInstance* const ThirdPersonAnimation = GetThirdPersonMesh()->GetAnimInstance())
	{
		ThirdPersonAnimation->Montage_Play(FireMontage);
	}
}

void AWPFinalPortalGunWeapon::InitializeFirstPersonSmoothing()
{
	USceneComponent* const SocketParent = GetFirstPersonMesh()->GetAttachParent();
	const FName SocketName = GetFirstPersonMesh()->GetAttachSocketName();
	if (!SocketParent || SocketName.IsNone())
	{
		return;
	}

	if (USkeletalMeshComponent* const CharacterMesh = Cast<USkeletalMeshComponent>(SocketParent))
	{
		CharacterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		CharacterMesh->bEnableUpdateRateOptimizations = false;
	}

	const FRotator RestRotation = GetFirstPersonMesh()->GetRelativeRotation();
	const FVector RestScale = GetFirstPersonMesh()->GetRelativeScale3D();
	FirstPersonSmoothingTarget->AttachToComponent(SocketParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

	const FTransform TargetRelativeTransform =
		FirstPersonSmoothingTarget->GetComponentTransform().GetRelativeTransform(GetRootComponent()->GetComponentTransform());
	FirstPersonSmoothingPivot->SetRelativeLocationAndRotation(TargetRelativeTransform.GetLocation(), TargetRelativeTransform.GetRotation());

	GetFirstPersonMesh()->AttachToComponent(FirstPersonSmoothingPivot, FAttachmentTransformRules::KeepWorldTransform);
	GetFirstPersonMesh()->SetRelativeLocationAndRotation(FirstPersonRestLocation, RestRotation);
	GetFirstPersonMesh()->SetRelativeScale3D(RestScale);
	AddTickPrerequisiteComponent(SocketParent);
}

void AWPFinalPortalGunWeapon::UpdateFirstPersonSmoothing(const float DeltaSeconds)
{
	if (!FirstPersonSmoothingPivot || !FirstPersonSmoothingTarget || !FirstPersonSmoothingTarget->GetAttachParent())
	{
		return;
	}

	const FTransform TargetRelativeTransform =
		FirstPersonSmoothingTarget->GetComponentTransform().GetRelativeTransform(GetRootComponent()->GetComponentTransform());
	const FVector SmoothedLocation = FMath::VInterpTo(FirstPersonSmoothingPivot->GetRelativeLocation(), TargetRelativeTransform.GetLocation(), DeltaSeconds,
		PortalConstants::Weapon::FirstPersonLocationInterpSpeed);
	const FQuat SmoothedRotation = FMath::QInterpTo(FirstPersonSmoothingPivot->GetRelativeRotation().Quaternion(), TargetRelativeTransform.GetRotation(),
		DeltaSeconds, PortalConstants::Weapon::FirstPersonRotationInterpSpeed);
	FirstPersonSmoothingPivot->SetRelativeLocationAndRotation(SmoothedLocation, SmoothedRotation);
}

void AWPFinalPortalGunWeapon::StartRecoil()
{
	RecoilStartAlpha = bRecoilActive ? CalculateRecoilAlpha() : 0.0f;
	RecoilElapsedSeconds = 0.0f;
	bRecoilActive = true;
}

float AWPFinalPortalGunWeapon::CalculateRecoilAlpha() const
{
	if (RecoilElapsedSeconds < PortalConstants::Weapon::RecoilKickTime)
	{
		const float LinearAlpha = FMath::Clamp(RecoilElapsedSeconds / PortalConstants::Weapon::RecoilKickTime, 0.0f, 1.0f);
		const float EaseOutAlpha = 1.0f - FMath::Pow(1.0f - LinearAlpha, PortalConstants::Weapon::RecoilEasePower);
		return FMath::Lerp(RecoilStartAlpha, 1.0f, EaseOutAlpha);
	}

	const float LinearAlpha =
		FMath::Clamp((RecoilElapsedSeconds - PortalConstants::Weapon::RecoilKickTime) / PortalConstants::Weapon::RecoilRecoveryTime, 0.0f, 1.0f);
	const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.0f - 2.0f * LinearAlpha);
	return 1.0f - SmoothAlpha;
}

void AWPFinalPortalGunWeapon::ApplyRecoil(const float RecoilAlpha)
{
	const FVector FirstPersonLocalOffset(0.0f, -PortalConstants::Weapon::RecoilDistance * RecoilAlpha, 0.0f);
	const FVector ThirdPersonLocalOffset(0.0f, -PortalConstants::Weapon::RecoilDistance * PortalConstants::Weapon::ThirdPersonRecoilRatio * RecoilAlpha, 0.0f);

	GetFirstPersonMesh()->SetRelativeLocation(FirstPersonRestLocation + GetFirstPersonMesh()->GetRelativeRotation().RotateVector(FirstPersonLocalOffset));
	GetThirdPersonMesh()->SetRelativeLocation(ThirdPersonRestLocation + GetThirdPersonMesh()->GetRelativeRotation().RotateVector(ThirdPersonLocalOffset));
}
