// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/EnergyBallReceiver.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gimmick/EnergyBall.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "Transit/WPTransitTags.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName EffectColorParameterName(TEXT("EffectColor"));
	const FName EmissiveStrengthParameterName(TEXT("EmissiveStrength"));
	const FName ShellSlotName(TEXT("Shell"));
	const FName MechanismSlotName(TEXT("Mechanism"));
	const FName OpticSlotName(TEXT("Optic"));
}

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
	ReceiverMesh->SetGenerateOverlapEvents(false);
	ReceiverMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> VisualMeshAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/EnergyBall/Meshes/SM_EnergyBallReceiver.SM_EnergyBallReceiver"));
	if (VisualMeshAsset.Succeeded())
	{
		EnergyBallVisualMesh = VisualMeshAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShellMaterialAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/EnergyBall/Materials/MI_EnergyBallShell.MI_EnergyBallShell"));
	if (ShellMaterialAsset.Succeeded())
	{
		ShellMaterial = ShellMaterialAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MechanismMaterialAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/EnergyBall/Materials/MI_EnergyBallMechanism.MI_EnergyBallMechanism"));
	if (MechanismMaterialAsset.Succeeded())
	{
		MechanismMaterial = MechanismMaterialAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OpticMaterialAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/EnergyBall/Materials/MI_EnergyBallOptic.MI_EnergyBallOptic"));
	if (OpticMaterialAsset.Succeeded())
	{
		OpticMaterial = OpticMaterialAsset.Object;
	}

	ApplyVisualAsset();

	ReceiverLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ReceiverLight"));
	ReceiverLight->SetupAttachment(DetectionSphere);
	ReceiverLight->SetRelativeLocation(FVector(64.0f, 0.0f, 0.0f));
	ReceiverLight->SetAttenuationRadius(340.0f);
	ReceiverLight->SetCastShadows(false);

	AcceptedBallClass = AEnergyBall::StaticClass();
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnergyBallReceiver::HandleDetectionBeginOverlap);
}

void AEnergyBallReceiver::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitializeVisualMaterials();
	UpdateReceiverVisuals(bStartActive);
}

void AEnergyBallReceiver::BeginPlay()
{
	Super::BeginPlay();
	InitializeVisualMaterials();
	UpdateReceiverVisuals(IsReceiverActive());

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

void AEnergyBallReceiver::HandleReceiverStateChanged(const bool bNewActive)
{
	Super::HandleReceiverStateChanged(bNewActive);
	UpdateReceiverVisuals(bNewActive);
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

void AEnergyBallReceiver::ApplyVisualAsset()
{
	if (!IsValid(ReceiverMesh)) return;

	ReceiverMesh->SetStaticMesh(EnergyBallVisualMesh);
	const int32 ShellIndex = ReceiverMesh->GetMaterialIndex(ShellSlotName);
	const int32 MechanismIndex = ReceiverMesh->GetMaterialIndex(MechanismSlotName);
	const int32 OpticIndex = ReceiverMesh->GetMaterialIndex(OpticSlotName);
	if (ShellIndex != INDEX_NONE && IsValid(ShellMaterial))
	{
		ReceiverMesh->SetMaterial(ShellIndex, ShellMaterial);
	}
	if (MechanismIndex != INDEX_NONE && IsValid(MechanismMaterial))
	{
		ReceiverMesh->SetMaterial(MechanismIndex, MechanismMaterial);
	}
	if (OpticIndex != INDEX_NONE && IsValid(OpticMaterial))
	{
		ReceiverMesh->SetMaterial(OpticIndex, OpticMaterial);
	}
}

void AEnergyBallReceiver::InitializeVisualMaterials()
{
	ApplyVisualAsset();
	ReceiverOpticMaterial = nullptr;
	if (!IsValid(ReceiverMesh)) return;

	const int32 OpticIndex = ReceiverMesh->GetMaterialIndex(OpticSlotName);
	if (OpticIndex != INDEX_NONE)
	{
		ReceiverOpticMaterial = ReceiverMesh->CreateAndSetMaterialInstanceDynamic(OpticIndex);
	}
}

void AEnergyBallReceiver::UpdateReceiverVisuals(const bool bActive)
{
	const FLinearColor StateColor = bActive ? ActiveOpticColor : InactiveOpticColor;
	if (IsValid(ReceiverOpticMaterial))
	{
		ReceiverOpticMaterial->SetVectorParameterValue(EffectColorParameterName, StateColor);
		ReceiverOpticMaterial->SetScalarParameterValue(
			EmissiveStrengthParameterName,
			bActive ? 9.0f : 0.25f);
	}

	if (IsValid(ReceiverLight))
	{
		ReceiverLight->SetLightColor(StateColor);
		ReceiverLight->SetIntensity(bActive ? 3800.0f : 45.0f);
	}
}
