// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/EnergyBallReceiver.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gimmick/EnergyBall.h"
#include "TimerManager.h"
#include "Transit/WPTransitTags.h"
#include "UObject/ConstructorHelpers.h"

AEnergyBallReceiver::AEnergyBallReceiver()
{
	PrimaryActorTick.bCanEverTick = false;
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	SetRootComponent(DetectionSphere);
	DetectionSphere->InitSphereRadius(75.0f);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	DetectionSphere->SetGenerateOverlapEvents(true);

	ReceiverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReceiverMesh"));
	ReceiverMesh->SetupAttachment(DetectionSphere);
	ReceiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReceiverMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ReceiverMesh->SetStaticMesh(CylinderMesh.Object);
	}

	AcceptedBallClass = AEnergyBall::StaticClass();
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnergyBallReceiver::HandleDetectionBeginOverlap);
}

void AEnergyBallReceiver::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bStartActive)
	{
		SetReceiverActive(true);
	}
}

void AEnergyBallReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DeactivationTimer);
	Super::EndPlay(EndPlayReason);
}

bool AEnergyBallReceiver::ReceiveEnergyBall(AEnergyBall* EnergyBall)
{
	if (!HasAuthority() || !IsValid(EnergyBall) || EnergyBall->IsConsumed()) return false;
	if (EnergyBall->ActorHasTag(WPTransitTags::Generated)) return false;
	if (AcceptedBallClass && !EnergyBall->IsA(AcceptedBallClass)) return false;

	OnEnergyBallReceived.Broadcast(EnergyBall);
	OnBallReceived(EnergyBall);
	SetReceiverActive(true);

	if (bConsumeBall)
	{
		EnergyBall->Consume(this);
	}

	return true;
}

void AEnergyBallReceiver::SetReceiverActive(const bool bNewActive)
{
	if (!HasAuthority()) return;

	Super::SetReceiverActive(bNewActive);
	if (bNewActive)
	{
		ScheduleDeactivation();
	}
	else
	{
		GetWorldTimerManager().ClearTimer(DeactivationTimer);
	}
}

void AEnergyBallReceiver::ScheduleDeactivation()
{
	GetWorldTimerManager().ClearTimer(DeactivationTimer);
	if (ActiveDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			DeactivationTimer,
			this,
			&AEnergyBallReceiver::DeactivateAfterDelay,
			ActiveDuration,
			false);
	}
}

void AEnergyBallReceiver::DeactivateAfterDelay()
{
	SetReceiverActive(false);
}

void AEnergyBallReceiver::HandleDetectionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	ReceiveEnergyBall(Cast<AEnergyBall>(OtherActor));
}
