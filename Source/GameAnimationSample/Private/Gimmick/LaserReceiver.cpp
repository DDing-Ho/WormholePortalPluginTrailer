// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/LaserReceiver.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gimmick/LaserEmitter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName EffectColorParameterName(TEXT("EffectColor"));
	const FName EmissiveStrengthParameterName(TEXT("EmissiveStrength"));
	const FName ShellSlotName(TEXT("Shell"));
	const FName MechanismSlotName(TEXT("Mechanism"));
	const FName OpticSlotName(TEXT("Optic"));
}

ALaserReceiver::ALaserReceiver()
{
	TargetSurface = CreateDefaultSubobject<UBoxComponent>(TEXT("TargetSurface"));
	SetRootComponent(TargetSurface);
	TargetSurface->InitBoxExtent(FVector(14.0f, 100.0f, 100.0f));
	TargetSurface->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TargetSurface->SetCollisionObjectType(ECC_WorldDynamic);
	TargetSurface->SetCollisionResponseToAllChannels(ECR_Ignore);
	TargetSurface->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
	TargetSurface->SetGenerateOverlapEvents(false);

	ReceiverBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReceiverBody"));
	ReceiverBody->SetupAttachment(TargetSurface);
	ReceiverBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReceiverBody->SetGenerateOverlapEvents(false);
	ReceiverBody->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> VisualMeshAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/Laser/Meshes/SM_LaserReceiver.SM_LaserReceiver"));
	if (VisualMeshAsset.Succeeded())
	{
		LaserVisualMesh = VisualMeshAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShellMaterialAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/Laser/Materials/MI_LaserShell.MI_LaserShell"));
	if (ShellMaterialAsset.Succeeded())
	{
		ShellMaterial = ShellMaterialAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MechanismMaterialAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/Laser/Materials/MI_LaserMechanism.MI_LaserMechanism"));
	if (MechanismMaterialAsset.Succeeded())
	{
		MechanismMaterial = MechanismMaterialAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OpticMaterialAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/Laser/Materials/MI_LaserOptic.MI_LaserOptic"));
	if (OpticMaterialAsset.Succeeded())
	{
		OpticMaterial = OpticMaterialAsset.Object;
	}

	ApplyVisualAsset();

	ReceiverLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ReceiverLight"));
	ReceiverLight->SetupAttachment(TargetSurface);
	ReceiverLight->SetRelativeLocation(FVector(24.0f, 0.0f, 0.0f));
	ReceiverLight->SetAttenuationRadius(280.0f);
	ReceiverLight->SetCastShadows(false);
}

void ALaserReceiver::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualAsset();
}

void ALaserReceiver::BeginPlay()
{
	Super::BeginPlay();
	InitializeVisualMaterials();
	UpdateReceiverVisuals(IsReceiverActive());
}

void ALaserReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveEmitters.Reset();
	if (HasAuthority())
	{
		Super::SetReceiverActive(false);
	}
	Super::EndPlay(EndPlayReason);
}

void ALaserReceiver::SetLaserContact(ALaserEmitter* LaserEmitter, const bool bInContact)
{
	if (!HasAuthority() || LaserEmitter == nullptr) return;

	const TWeakObjectPtr<ALaserEmitter> EmitterKey(LaserEmitter);
	if (bInContact)
	{
		ActiveEmitters.Add(EmitterKey);
	}
	else
	{
		ActiveEmitters.Remove(EmitterKey);
	}

	RefreshContactState();
}

int32 ALaserReceiver::GetActiveLaserCount() const
{
	int32 ValidEmitterCount = 0;
	for (const TWeakObjectPtr<ALaserEmitter>& LaserEmitter : ActiveEmitters)
	{
		if (LaserEmitter.IsValid())
		{
			++ValidEmitterCount;
		}
	}
	return ValidEmitterCount;
}

void ALaserReceiver::HandleReceiverStateChanged(const bool bNewActive)
{
	Super::HandleReceiverStateChanged(bNewActive);
	UpdateReceiverVisuals(bNewActive);
}

void ALaserReceiver::RefreshContactState()
{
	for (auto EmitterIterator = ActiveEmitters.CreateIterator(); EmitterIterator; ++EmitterIterator)
	{
		if (!EmitterIterator->IsValid())
		{
			EmitterIterator.RemoveCurrent();
		}
	}

	SetReceiverActive(!ActiveEmitters.IsEmpty());
}

void ALaserReceiver::InitializeVisualMaterials()
{
	ApplyVisualAsset();
	ReceiverOpticMaterial = nullptr;
	if (IsValid(ReceiverBody))
	{
		const int32 OpticMaterialIndex = ReceiverBody->GetMaterialIndex(OpticSlotName);
		if (OpticMaterialIndex != INDEX_NONE)
		{
			ReceiverOpticMaterial = ReceiverBody->CreateAndSetMaterialInstanceDynamic(OpticMaterialIndex);
		}
	}
}

void ALaserReceiver::UpdateReceiverVisuals(const bool bActive)
{
	const FLinearColor StateColor = bActive ? ActiveColor : InactiveColor;
	if (IsValid(ReceiverOpticMaterial))
	{
		ReceiverOpticMaterial->SetVectorParameterValue(EffectColorParameterName, StateColor);
		ReceiverOpticMaterial->SetScalarParameterValue(
			EmissiveStrengthParameterName,
			bActive ? 8.0f : 0.35f);
	}

	if (IsValid(ReceiverLight))
	{
		ReceiverLight->SetLightColor(StateColor);
		ReceiverLight->SetIntensity(bActive ? 3200.0f : 70.0f);
	}
}

void ALaserReceiver::ApplyVisualAsset()
{
	if (!IsValid(ReceiverBody)) return;

	ReceiverBody->SetStaticMesh(LaserVisualMesh);
	const int32 ShellIndex = ReceiverBody->GetMaterialIndex(ShellSlotName);
	const int32 MechanismIndex = ReceiverBody->GetMaterialIndex(MechanismSlotName);
	const int32 OpticIndex = ReceiverBody->GetMaterialIndex(OpticSlotName);
	if (ShellIndex != INDEX_NONE && IsValid(ShellMaterial))
	{
		ReceiverBody->SetMaterial(ShellIndex, ShellMaterial);
	}
	if (MechanismIndex != INDEX_NONE && IsValid(MechanismMaterial))
	{
		ReceiverBody->SetMaterial(MechanismIndex, MechanismMaterial);
	}
	if (OpticIndex != INDEX_NONE && IsValid(OpticMaterial))
	{
		ReceiverBody->SetMaterial(OpticIndex, OpticMaterial);
	}
}
