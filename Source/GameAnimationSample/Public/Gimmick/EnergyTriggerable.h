// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EnergyTriggerable.generated.h"

class AEnergyReceiverBase;

/**
 * Gameplay contract for Actors controlled by an energy receiver.
 *
 * Add this interface to a Blueprint (for example, a door or moving platform) and implement
 * Set Energy Trigger Active to react without requiring a Level Blueprint binding.
 */
UINTERFACE(BlueprintType, meta = (DisplayName = "Energy Triggerable"))
class GAMEANIMATIONSAMPLE_API UEnergyTriggerable : public UInterface
{
	GENERATED_BODY()
};

class GAMEANIMATIONSAMPLE_API IEnergyTriggerable
{
	GENERATED_BODY()

public:
	/** Called whenever a connected Receiver changes its active state. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Animation Sample|Energy Receiver")
	void SetEnergyTriggerActive(bool bActive, AEnergyReceiverBase* SourceReceiver);
};
