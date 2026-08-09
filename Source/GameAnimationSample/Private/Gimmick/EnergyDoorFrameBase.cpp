// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/EnergyDoorFrameBase.h"

#include "Components/TimelineComponent.h"
#include "Gimmick/EnergyReceiverBase.h"

namespace
{
	const FName DoorControlTimelineName(TEXT("Door Control"));
}

AEnergyDoorFrameBase::AEnergyDoorFrameBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnergyDoorFrameBase::BeginPlay()
{
	Super::BeginPlay();

	CacheDoorTimeline();
	BindReceivers();
	EvaluateReceiverState();
}

void AEnergyDoorFrameBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindReceivers();
	Super::EndPlay(EndPlayReason);
}

void AEnergyDoorFrameBase::NotifyActorBeginOverlap(AActor* OtherActor)
{
	// Energy doors are controlled only by their configured receivers.
}

void AEnergyDoorFrameBase::NotifyActorEndOverlap(AActor* OtherActor)
{
	// Energy doors are controlled only by their configured receivers.
}

void AEnergyDoorFrameBase::HandleReceiverActivationChanged(const bool bIsActive)
{
	EvaluateReceiverState();
}

void AEnergyDoorFrameBase::HandleReceiverEndPlay(AActor* Actor, const EEndPlayReason::Type EndPlayReason)
{
	// An AND-controlled door must fail closed as soon as any configured source leaves play.
	ApplyDoorRequest(false);
}

void AEnergyDoorFrameBase::BindReceivers()
{
	if (SourceReceivers.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Energy door '%s' has no Source Receivers configured and will remain closed."),
			*GetName());
		return;
	}

	for (AEnergyReceiverBase* Receiver : SourceReceivers)
	{
		if (IsValid(Receiver))
		{
			Receiver->OnActivationChanged.AddUniqueDynamic(
				this,
				&AEnergyDoorFrameBase::HandleReceiverActivationChanged);
			Receiver->OnEndPlay.AddUniqueDynamic(
				this,
				&AEnergyDoorFrameBase::HandleReceiverEndPlay);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Energy door '%s' has an invalid Source Receiver and will remain closed."),
				*GetName());
		}
	}
}

void AEnergyDoorFrameBase::UnbindReceivers()
{
	for (AEnergyReceiverBase* Receiver : SourceReceivers)
	{
		if (IsValid(Receiver))
		{
			Receiver->OnActivationChanged.RemoveDynamic(
				this,
				&AEnergyDoorFrameBase::HandleReceiverActivationChanged);
			Receiver->OnEndPlay.RemoveDynamic(
				this,
				&AEnergyDoorFrameBase::HandleReceiverEndPlay);
		}
	}
}

void AEnergyDoorFrameBase::CacheDoorTimeline()
{
	DoorTimeline = nullptr;

	TArray<UTimelineComponent*> Timelines;
	GetComponents(Timelines);
	for (UTimelineComponent* Timeline : Timelines)
	{
		if (IsValid(Timeline) && Timeline->GetFName() == DoorControlTimelineName)
		{
			DoorTimeline = Timeline;
			break;
		}
	}

	if (!IsValid(DoorTimeline) && Timelines.Num() == 1)
	{
		DoorTimeline = Timelines[0];
	}

	if (!IsValid(DoorTimeline))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Energy door '%s' could not find the Door Control timeline."),
			*GetName());
	}
}

void AEnergyDoorFrameBase::EvaluateReceiverState()
{
	bool bShouldOpen = !SourceReceivers.IsEmpty();
	for (const AEnergyReceiverBase* Receiver : SourceReceivers)
	{
		if (!IsValid(Receiver) || !Receiver->IsReceiverActive())
		{
			bShouldOpen = false;
			break;
		}
	}

	ApplyDoorRequest(bShouldOpen);
}

void AEnergyDoorFrameBase::ApplyDoorRequest(const bool bShouldOpen)
{
	if (bHasAppliedDoorRequest && bDoorOpenRequested == bShouldOpen)
	{
		return;
	}

	bHasAppliedDoorRequest = true;
	bDoorOpenRequested = bShouldOpen;

	if (!IsValid(DoorTimeline))
	{
		return;
	}

	if (bShouldOpen)
	{
		DoorTimeline->Play();
	}
	else
	{
		DoorTimeline->Reverse();
	}
}
