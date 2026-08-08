// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/FirstPersonPhysicsGrabComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UFirstPersonPhysicsGrabComponent::UFirstPersonPhysicsGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bSoftAngularConstraint = false;
}

bool UFirstPersonPhysicsGrabComponent::GrabActor()
{
	if (IsHoldingActor())
	{
		return false;
	}

	UWorld* const World = GetWorld();
	AActor* const OwnerActor = GetOwner();
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!World || !IsValid(OwnerActor) || !GetOwnerView(ViewLocation, ViewRotation))
	{
		return false;
	}

	const FVector ViewDirection = ViewRotation.Vector();
	const FVector TraceEnd = ViewLocation + ViewDirection * FMath::Max(GrabRange, 0.0f);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FirstPersonPhysicsGrab), bTraceComplex, OwnerActor);
	FHitResult HitResult;
	if (!World->LineTraceSingleByObjectType(HitResult, ViewLocation, TraceEnd, ObjectQueryParams, QueryParams))
	{
		return false;
	}

	UPrimitiveComponent* const HitComponent = HitResult.GetComponent();
	if (!IsValid(HitComponent) || !HitComponent->IsSimulatingPhysics(HitResult.BoneName))
	{
		return false;
	}

	bSoftAngularConstraint = false;
	GrabComponentAtLocationWithRotation(HitComponent, HitResult.BoneName, HitResult.ImpactPoint, FRotator::ZeroRotator);

	if (!IsHoldingActor())
	{
		return false;
	}

	SetTargetLocationAndRotation(
		ViewLocation + ViewDirection * FMath::Max(HoldDistance, 0.0f),
		FRotator::ZeroRotator);
	SetComponentTickEnabled(true);
	return true;
}

void UFirstPersonPhysicsGrabComponent::DropActor()
{
	ReleaseComponent();
	SetComponentTickEnabled(false);
}

bool UFirstPersonPhysicsGrabComponent::ToggleGrab()
{
	if (IsHoldingActor())
	{
		DropActor();
		return true;
	}

	return GrabActor();
}

bool UFirstPersonPhysicsGrabComponent::IsHoldingActor() const
{
	return IsValid(GetGrabbedComponent());
}

void UFirstPersonPhysicsGrabComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* const ThisTickFunction)
{
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!IsHoldingActor() || !GetOwnerView(ViewLocation, ViewRotation))
	{
		DropActor();
	}
	else
	{
		SetTargetLocationAndRotation(
			ViewLocation + ViewRotation.Vector() * FMath::Max(HoldDistance, 0.0f),
			FRotator::ZeroRotator);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UFirstPersonPhysicsGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DropActor();
	Super::EndPlay(EndPlayReason);
}

bool UFirstPersonPhysicsGrabComponent::GetOwnerView(FVector& OutLocation, FRotator& OutRotation) const
{
	AActor* const OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return false;
	}

	if (const APawn* const Pawn = Cast<APawn>(OwnerActor))
	{
		if (const APlayerController* const PlayerController = Cast<APlayerController>(Pawn->GetController()))
		{
			PlayerController->GetPlayerViewPoint(OutLocation, OutRotation);
			return true;
		}
	}

	OwnerActor->GetActorEyesViewPoint(OutLocation, OutRotation);
	return true;
}
