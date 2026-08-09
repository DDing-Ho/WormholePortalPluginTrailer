// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnergyDoorFrameBase.generated.h"

class AEnergyReceiverBase;
class UTimelineComponent;

/**
 * Native receiver-state controller for the BP_EnergyDoorFrame asset.
 *
 * The Blueprint keeps the original BP_DoorFrame construction script and Door Control
 * timeline. This base class only selects receiver inputs and drives that timeline.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisplayName = "Energy Door Frame Base"))
class GAMEANIMATIONSAMPLE_API AEnergyDoorFrameBase : public AActor
{
	GENERATED_BODY()

public:
	AEnergyDoorFrameBase();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

private:
	UFUNCTION()
	void HandleReceiverActivationChanged(bool bIsActive);

	UFUNCTION()
	void HandleReceiverEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	void BindReceivers();
	void UnbindReceivers();
	void CacheDoorTimeline();
	void EvaluateReceiverState();
	void ApplyDoorRequest(bool bShouldOpen);

	/** Every configured Receiver must be active for this door to open. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Door", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AEnergyReceiverBase>> SourceReceivers;

	UPROPERTY(Transient)
	TObjectPtr<UTimelineComponent> DoorTimeline;

	bool bHasAppliedDoorRequest = false;
	bool bDoorOpenRequested = false;
};
