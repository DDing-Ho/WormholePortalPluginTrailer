// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnergyReceiverBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnergyReceiverActivationChanged, bool, bIsActive);

/**
 * Shared trigger state for receivers powered by Energy Balls, lasers, or future energy sources.
 *
 * Bind to On Activation Changed, override the Blueprint events, or add Connected Actors
 * that implement Energy Triggerable.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class GAMEANIMATIONSAMPLE_API AEnergyReceiverBase : public AActor
{
	GENERATED_BODY()

public:
	AEnergyReceiverBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Controls the shared trigger state. Only the server may change it. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Energy Receiver")
	virtual void SetReceiverActive(bool bNewActive);

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Energy Receiver")
	bool IsReceiverActive() const { return bIsActive; }

	UPROPERTY(BlueprintAssignable, Category = "Game Animation Sample|Energy Receiver")
	FEnergyReceiverActivationChanged OnActivationChanged;

protected:
	/** Called on the server and clients after the replicated state changes. */
	virtual void HandleReceiverStateChanged(bool bNewActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Animation Sample|Energy Receiver", meta = (DisplayName = "On Receiver Activated"))
	void OnReceiverActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Animation Sample|Energy Receiver", meta = (DisplayName = "On Receiver Deactivated"))
	void OnReceiverDeactivated();

private:
	void NotifyActivationChanged();

	UFUNCTION()
	void OnRep_IsActive();

	/** Actors notified through the Energy Triggerable interface. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Receiver|Trigger", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AActor>> ConnectedActors;

	UPROPERTY(ReplicatedUsing = OnRep_IsActive, VisibleInstanceOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Receiver|Trigger", meta = (AllowPrivateAccess = "true"))
	bool bIsActive = false;
};
