// Copyright 2026 Team Beaver. All Rights Reserved.

#include "WPFinalPortalGunComponent.h"

#include "WPFinalPortalGunConstants.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/RotationMatrix.h"
#include "WormholePortalActor.h"

namespace PortalConstants = WPFinalPortalGunConstants;

UWPFinalPortalGunComponent::UWPFinalPortalGunComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWPFinalPortalGunComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateGrowth(DeltaTime);
}

void UWPFinalPortalGunComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPortals();
	Super::EndPlay(EndPlayReason);
}

bool UWPFinalPortalGunComponent::PlacePortalA(const FHitResult& HitResult)
{
	return bCanSpawnPortalA && PlacePortal(HitResult, BluePortal, BluePortalGrowthSeconds, bBluePortalIsGrowing);
}

bool UWPFinalPortalGunComponent::PlacePortalB(const FHitResult& HitResult)
{
	return bCanSpawnPortalB && PlacePortal(HitResult, OrangePortal, OrangePortalGrowthSeconds, bOrangePortalIsGrowing);
}

void UWPFinalPortalGunComponent::ClearPortals()
{
	ReleasePortal(BluePortal, bBluePortalIsGrowing);
	ReleasePortal(OrangePortal, bOrangePortalIsGrowing);
	BluePortalGrowthSeconds = 0.0f;
	OrangePortalGrowthSeconds = 0.0f;
	SetComponentTickEnabled(false);
}

bool UWPFinalPortalGunComponent::HasPortalA() const
{
	return IsValid(BluePortal.Get());
}

bool UWPFinalPortalGunComponent::HasPortalB() const
{
	return IsValid(OrangePortal.Get());
}

void UWPFinalPortalGunComponent::CyclePortalSpawnDirection(const float WheelDelta)
{
	if (FMath::IsNearlyZero(WheelDelta))
	{
		return;
	}

	const int32 DirectionCount = static_cast<int32>(EWPFinalPortalSpawnDirection::Count);
	const int32 CurrentDirection = FMath::Clamp(static_cast<int32>(PortalSpawnDirection), 0, DirectionCount - 1);
	const int32 DirectionStep = WheelDelta > 0.0f ? 1 : -1;
	PortalSpawnDirection = static_cast<EWPFinalPortalSpawnDirection>((CurrentDirection + DirectionStep + DirectionCount) % DirectionCount);
}

bool UWPFinalPortalGunComponent::PlacePortal(const FHitResult& HitResult, TObjectPtr<AWormholePortalActor>& PortalSlot, float& GrowthElapsed, bool& bIsGrowing)
{
	AActor* const OwnerActor = GetOwner();
	UWorld* const World = GetWorld();
	if (!IsValid(OwnerActor) || !World || !HitResult.bBlockingHit)
	{
		return false;
	}

	const FVector SurfaceNormal = HitResult.ImpactNormal.GetSafeNormal();
	if (SurfaceNormal.IsNearlyZero())
	{
		return false;
	}

	const FVector SpawnLocation = HitResult.ImpactPoint + SurfaceNormal * PortalRadius * PortalConstants::Portal::SurfaceOffsetRatio;
	const FRotator SpawnRotation = FRotationMatrix::MakeFromX(ToWorldVector(PortalSpawnDirection)).Rotator();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerActor;
	SpawnParameters.Instigator = OwnerActor->GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWormholePortalActor* const NewPortal =
		World->SpawnActor<AWormholePortalActor>(AWormholePortalActor::StaticClass(), SpawnLocation, SpawnRotation, SpawnParameters);
	if (!IsValid(NewPortal))
	{
		return false;
	}

	NewPortal->SetActorScale3D(FVector::OneVector);
	InitializeFinalPhysicalMetric(NewPortal);

	ReleasePortal(PortalSlot, bIsGrowing);
	PortalSlot = NewPortal;
	GrowthElapsed = 0.0f;

	const float StartScale =
		GrowthDuration > UE_KINDA_SMALL_NUMBER ? FMath::Clamp(InitialGrowthScale, PortalConstants::Portal::MinimumGrowthScale, 1.0f) : 1.0f;
	bIsGrowing = StartScale < 1.0f;
	ApplyGrowthScale(NewPortal, StartScale);
	SetComponentTickEnabled(bBluePortalIsGrowing || bOrangePortalIsGrowing || bIsGrowing);

	LinkPortals();
	return true;
}

void UWPFinalPortalGunComponent::LinkPortals()
{
	AWormholePortalActor* const Blue = BluePortal.Get();
	AWormholePortalActor* const Orange = OrangePortal.Get();
	if (IsValid(Blue) && IsValid(Orange) && Blue->GetLinkedPortal() != Orange)
	{
		Blue->SetLinkedPortal(Orange);
	}
}

void UWPFinalPortalGunComponent::UpdateGrowth(const float DeltaSeconds)
{
	UpdateOnePortalGrowth(BluePortal.Get(), DeltaSeconds, BluePortalGrowthSeconds, bBluePortalIsGrowing);
	UpdateOnePortalGrowth(OrangePortal.Get(), DeltaSeconds, OrangePortalGrowthSeconds, bOrangePortalIsGrowing);
	SetComponentTickEnabled(bBluePortalIsGrowing || bOrangePortalIsGrowing);
}

void UWPFinalPortalGunComponent::UpdateOnePortalGrowth(AWormholePortalActor* const Portal, const float DeltaSeconds, float& ElapsedSeconds, bool& bIsGrowing)
{
	if (!bIsGrowing)
	{
		return;
	}

	if (!IsValid(Portal))
	{
		bIsGrowing = false;
		ElapsedSeconds = 0.0f;
		return;
	}

	ElapsedSeconds += FMath::Max(DeltaSeconds, 0.0f);
	const float LinearAlpha = FMath::Clamp(ElapsedSeconds / FMath::Max(GrowthDuration, UE_KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float StartScale = FMath::Clamp(InitialGrowthScale, PortalConstants::Portal::MinimumGrowthScale, 1.0f);
	ApplyGrowthScale(Portal, FMath::Lerp(StartScale, 1.0f, EaseGrowth(LinearAlpha)));

	if (LinearAlpha >= 1.0f)
	{
		bIsGrowing = false;
	}
}

void UWPFinalPortalGunComponent::InitializeFinalPhysicalMetric(AWormholePortalActor* const Portal) const
{
	if (IsValid(Portal))
	{
		Portal->InitializePhysicalMetric(PortalRadius, ThroatHalfLength, TransitionLength);
	}
}

void UWPFinalPortalGunComponent::ApplyGrowthScale(AWormholePortalActor* const Portal, const float Scale) const
{
	if (!IsValid(Portal))
	{
		return;
	}

	const float SafeScale = FMath::Clamp(Scale, PortalConstants::Portal::MinimumGrowthScale, 1.0f);
	if (GrowthMode == EWPFinalPortalGrowthMode::VisualOnly)
	{
		Portal->SetPortalVisualScale(SafeScale);
		return;
	}

	Portal->SetPortalVisualScale(1.0f);
	Portal->SetUniformPhysicalMetricScale(SafeScale);
}

void UWPFinalPortalGunComponent::ReleasePortal(TObjectPtr<AWormholePortalActor>& Portal, bool& bIsGrowing)
{
	if (IsValid(Portal.Get()))
	{
		Portal->Destroy();
	}

	Portal = nullptr;
	bIsGrowing = false;
}

float UWPFinalPortalGunComponent::EaseGrowth(const float LinearAlpha)
{
	const float SafeAlpha = FMath::Clamp(LinearAlpha, 0.0f, 1.0f);
	return 1.0f - FMath::Pow(1.0f - SafeAlpha, PortalConstants::Portal::GrowthEasePower);
}

FVector UWPFinalPortalGunComponent::ToWorldVector(const EWPFinalPortalSpawnDirection Direction)
{
	switch (Direction)
	{
	case EWPFinalPortalSpawnDirection::PositiveX:
		return FVector::ForwardVector;
	case EWPFinalPortalSpawnDirection::NegativeX:
		return FVector::BackwardVector;
	case EWPFinalPortalSpawnDirection::PositiveY:
		return FVector::RightVector;
	case EWPFinalPortalSpawnDirection::NegativeY:
		return FVector::LeftVector;
	case EWPFinalPortalSpawnDirection::PositiveZ:
		return FVector::UpVector;
	case EWPFinalPortalSpawnDirection::NegativeZ:
		return FVector::DownVector;
	default:
		return FVector::ForwardVector;
	}
}
