// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/EnergyBallEmitter.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Gimmick/EnergyBall.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName EffectColorParameterName(TEXT("EffectColor"));
	const FName EmissiveStrengthParameterName(TEXT("EmissiveStrength"));
	const FName ShellSlotName(TEXT("Shell"));
	const FName MechanismSlotName(TEXT("Mechanism"));
	const FName OpticSlotName(TEXT("Optic"));
}

AEnergyBallEmitter::AEnergyBallEmitter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EmitterCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("EmitterCollision"));
	EmitterCollision->SetupAttachment(SceneRoot);
	EmitterCollision->InitBoxExtent(FVector(25.0f, 55.0f, 55.0f));
	EmitterCollision->SetRelativeLocation(FVector(20.0f, 0.0f, 0.0f));
	EmitterCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	EmitterCollision->SetGenerateOverlapEvents(false);

	EmitterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterMesh"));
	EmitterMesh->SetupAttachment(SceneRoot);
	EmitterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EmitterMesh->SetGenerateOverlapEvents(false);
	EmitterMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> VisualMeshAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/EnergyBall/Meshes/SM_EnergyBallEmitter.SM_EnergyBallEmitter"));
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

	Muzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(SceneRoot);
	Muzzle->SetRelativeLocation(FVector(70.0f, 0.0f, 0.0f));
	Muzzle->SetArrowColor(FColor::Cyan);
	Muzzle->SetArrowSize(1.5f);

	EmitterLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EmitterLight"));
	EmitterLight->SetupAttachment(SceneRoot);
	EmitterLight->SetRelativeLocation(FVector(66.0f, 0.0f, 0.0f));
	EmitterLight->SetAttenuationRadius(300.0f);
	EmitterLight->SetCastShadows(false);

	EnergyBallClass = AEnergyBall::StaticClass();
}

void AEnergyBallEmitter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitializeVisualMaterials();
	UpdateEmitterVisuals(bStartActive);
}

void AEnergyBallEmitter::BeginPlay()
{
	Super::BeginPlay();
	InitializeVisualMaterials();
	UpdateEmitterVisuals(bIsFiring);

	if (HasAuthority() && bStartActive)
	{
		StartFiring();
	}
}

void AEnergyBallEmitter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FireTimer);
	Super::EndPlay(EndPlayReason);
}

void AEnergyBallEmitter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEnergyBallEmitter, bIsFiring);
}

void AEnergyBallEmitter::StartFiring()
{
	if (!HasAuthority() || bIsFiring) return;

	bIsFiring = true;
	UpdateEmitterVisuals(true);
	ForceNetUpdate();

	if (bFireImmediately)
	{
		FireEnergyBall();
	}

	if (FireInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			FireTimer,
			this,
			&AEnergyBallEmitter::HandleFireTimer,
			FMath::Max(FireInterval, 0.05f),
			true);
	}
}

void AEnergyBallEmitter::StopFiring()
{
	if (!HasAuthority()) return;

	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(FireTimer);
	UpdateEmitterVisuals(false);
	ForceNetUpdate();
}

AEnergyBall* AEnergyBallEmitter::FireEnergyBall()
{
	if (!HasAuthority() || !EnergyBallClass || !IsValid(Muzzle)) return nullptr;

	PruneInactiveBalls();
	if (MaxActiveBalls > 0 && ActiveBalls.Num() >= MaxActiveBalls) return nullptr;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEnergyBall* EnergyBall = GetWorld()->SpawnActor<AEnergyBall>(
		EnergyBallClass,
		Muzzle->GetComponentTransform(),
		SpawnParameters);

	if (!IsValid(EnergyBall)) return nullptr;

	ActiveBalls.Add(EnergyBall);
	EnergyBall->Launch(Muzzle->GetForwardVector(), BallSpeed);
	OnEnergyBallFired.Broadcast(EnergyBall);
	return EnergyBall;
}

void AEnergyBallEmitter::HandleFireTimer()
{
	FireEnergyBall();
}

void AEnergyBallEmitter::PruneInactiveBalls()
{
	ActiveBalls.RemoveAll([](const TWeakObjectPtr<AEnergyBall>& EnergyBall)
	{
		return !EnergyBall.IsValid();
	});
}

void AEnergyBallEmitter::OnRep_IsFiring()
{
	UpdateEmitterVisuals(bIsFiring);
}

void AEnergyBallEmitter::ApplyVisualAsset()
{
	if (!IsValid(EmitterMesh)) return;

	EmitterMesh->SetStaticMesh(EnergyBallVisualMesh);
	const int32 ShellIndex = EmitterMesh->GetMaterialIndex(ShellSlotName);
	const int32 MechanismIndex = EmitterMesh->GetMaterialIndex(MechanismSlotName);
	const int32 OpticIndex = EmitterMesh->GetMaterialIndex(OpticSlotName);
	if (ShellIndex != INDEX_NONE && IsValid(ShellMaterial))
	{
		EmitterMesh->SetMaterial(ShellIndex, ShellMaterial);
	}
	if (MechanismIndex != INDEX_NONE && IsValid(MechanismMaterial))
	{
		EmitterMesh->SetMaterial(MechanismIndex, MechanismMaterial);
	}
	if (OpticIndex != INDEX_NONE && IsValid(OpticMaterial))
	{
		EmitterMesh->SetMaterial(OpticIndex, OpticMaterial);
	}
}

void AEnergyBallEmitter::InitializeVisualMaterials()
{
	ApplyVisualAsset();
	EmitterOpticMaterial = nullptr;
	if (!IsValid(EmitterMesh)) return;

	const int32 OpticIndex = EmitterMesh->GetMaterialIndex(OpticSlotName);
	if (OpticIndex != INDEX_NONE)
	{
		EmitterOpticMaterial = EmitterMesh->CreateAndSetMaterialInstanceDynamic(OpticIndex);
	}
}

void AEnergyBallEmitter::UpdateEmitterVisuals(const bool bActive)
{
	const FLinearColor StateColor = bActive ? ActiveOpticColor : InactiveOpticColor;
	if (IsValid(EmitterOpticMaterial))
	{
		EmitterOpticMaterial->SetVectorParameterValue(EffectColorParameterName, StateColor);
		EmitterOpticMaterial->SetScalarParameterValue(
			EmissiveStrengthParameterName,
			bActive ? 9.0f : 0.25f);
	}

	if (IsValid(EmitterLight))
	{
		EmitterLight->SetLightColor(StateColor);
		EmitterLight->SetIntensity(bActive ? 4200.0f : 45.0f);
	}
}
