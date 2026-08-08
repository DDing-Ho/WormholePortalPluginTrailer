// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "FirstPersonPhysicsGrabComponent.generated.h"

/**
 * Physics-handle based grabber that traces from its owner's view and keeps the
 * grabbed body at a configurable distance in front of the player.
 */
UCLASS(ClassGroup=(Gameplay), BlueprintType, meta=(BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UFirstPersonPhysicsGrabComponent : public UPhysicsHandleComponent
{
	GENERATED_BODY()

public:
	UFirstPersonPhysicsGrabComponent();

	/** Tries to grab the first simulating physics body in the center of the view. */
	UFUNCTION(BlueprintCallable, Category="Shooter|Grab")
	bool GrabActor();

	/** Releases the currently grabbed body. */
	UFUNCTION(BlueprintCallable, Category="Shooter|Grab")
	void DropActor();

	/** Drops the current body, or tries to grab one when the handle is empty. */
	UFUNCTION(BlueprintCallable, Category="Shooter|Grab")
	bool ToggleGrab();

	/** Returns whether this component currently owns a valid physics constraint. */
	UFUNCTION(BlueprintPure, Category="Shooter|Grab")
	bool IsHoldingActor() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool GetOwnerView(FVector& OutLocation, FRotator& OutRotation) const;

	/** Maximum distance in which a physics body can be selected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Grab", meta=(AllowPrivateAccess="true", ClampMin="0.0", Units="cm"))
	float GrabRange = 500.0f;

	/** Distance at which the grabbed body is constrained in front of the view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Grab", meta=(AllowPrivateAccess="true", ClampMin="0.0", Units="cm"))
	float HoldDistance = 500.0f;

	/** Uses complex collision for the view trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Grab", meta=(AllowPrivateAccess="true"))
	bool bTraceComplex = false;
};
