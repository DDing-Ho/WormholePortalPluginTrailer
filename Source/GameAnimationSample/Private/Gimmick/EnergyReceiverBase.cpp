// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/EnergyReceiverBase.h"

#include "Gimmick/EnergyTriggerable.h"
#include "Net/UnrealNetwork.h"

AEnergyReceiverBase::AEnergyReceiverBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void AEnergyReceiverBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEnergyReceiverBase, bIsActive);
}

void AEnergyReceiverBase::SetReceiverActive(const bool bNewActive)
{
	if (!HasAuthority() || bIsActive == bNewActive)
	{
		return;
	}

	bIsActive = bNewActive;
	NotifyActivationChanged();
	ForceNetUpdate();
}

void AEnergyReceiverBase::HandleReceiverStateChanged(const bool bNewActive)
{
}

void AEnergyReceiverBase::NotifyActivationChanged()
{
	OnActivationChanged.Broadcast(bIsActive);

	if (bIsActive)
	{
		OnReceiverActivated();
	}
	else
	{
		OnReceiverDeactivated();
	}

	for (AActor* ConnectedActor : ConnectedActors)
	{
		if (IsValid(ConnectedActor) && ConnectedActor->GetClass()->ImplementsInterface(UEnergyTriggerable::StaticClass()))
		{
			IEnergyTriggerable::Execute_SetEnergyTriggerActive(ConnectedActor, bIsActive, this);
		}
	}

	HandleReceiverStateChanged(bIsActive);
}

void AEnergyReceiverBase::OnRep_IsActive()
{
	NotifyActivationChanged();
}
