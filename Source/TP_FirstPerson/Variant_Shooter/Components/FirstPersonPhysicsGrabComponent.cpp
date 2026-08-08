// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/FirstPersonPhysicsGrabComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/WPTransitSubsystem.h"
#include "Transit/WPTransitTypes.h"

UFirstPersonPhysicsGrabComponent::UFirstPersonPhysicsGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bSoftAngularConstraint = false;
}

void UFirstPersonPhysicsGrabComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UWPTransitSubsystem* TransitSubsystem = World->GetSubsystem<UWPTransitSubsystem>())
		{
			TransitStartedHandle = TransitSubsystem->OnTransitStarted().AddUObject(
				this,
				&UFirstPersonPhysicsGrabComponent::HandleTransitStarted);
		}
	}
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
	bAimHeldObjectWithView = HitComponent->ComponentHasTag(AimWithViewComponentTag);
	const FVector GrabLocation = bAimHeldObjectWithView
		? HitComponent->GetCenterOfMass(HitResult.BoneName)
		: HitResult.ImpactPoint;
	const FRotator TargetRotation = bAimHeldObjectWithView ? ViewRotation : FRotator::ZeroRotator;
	GrabComponentAtLocationWithRotation(HitComponent, HitResult.BoneName, GrabLocation, TargetRotation);

	if (!IsHoldingActor())
	{
		bAimHeldObjectWithView = false;
		return false;
	}

	const float ActiveHoldDistance = bAimHeldObjectWithView ? AimableHoldDistance : HoldDistance;
	SetTargetLocationAndRotation(
		ViewLocation + ViewDirection * FMath::Max(ActiveHoldDistance, 0.0f),
		TargetRotation);
	SetComponentTickEnabled(true);
	return true;
}

void UFirstPersonPhysicsGrabComponent::DropActor()
{
	ReleaseComponent();
	bAimHeldObjectWithView = false;
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
		const float ActiveHoldDistance = bAimHeldObjectWithView ? AimableHoldDistance : HoldDistance;
		SetTargetLocationAndRotation(
			ViewLocation + ViewRotation.Vector() * FMath::Max(ActiveHoldDistance, 0.0f),
			bAimHeldObjectWithView ? ViewRotation : FRotator::ZeroRotator);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UFirstPersonPhysicsGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TransitStartedHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UWPTransitSubsystem* TransitSubsystem = World->GetSubsystem<UWPTransitSubsystem>())
			{
				TransitSubsystem->OnTransitStarted().Remove(TransitStartedHandle);
			}
		}
		TransitStartedHandle.Reset();
	}

	DropActor();
	Super::EndPlay(EndPlayReason);
}

void UFirstPersonPhysicsGrabComponent::HandleTransitStarted(const FWPTransitEvent& Event)
{
	UPrimitiveComponent* HeldComponent = GetGrabbedComponent();
	if (IsValid(HeldComponent) && HeldComponent->GetOwner() == Event.Actor.Get())
	{
		DropActor();
	}
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
