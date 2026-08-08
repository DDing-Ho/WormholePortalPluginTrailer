// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/EnergyBallEmitter.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Gimmick/EnergyBall.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AEnergyBallEmitter::AEnergyBallEmitter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EmitterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterMesh"));
	EmitterMesh->SetupAttachment(SceneRoot);
	EmitterMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	EmitterMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 0.3f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		EmitterMesh->SetStaticMesh(CylinderMesh.Object);
	}

	Muzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(SceneRoot);
	Muzzle->SetRelativeLocation(FVector(70.0f, 0.0f, 0.0f));
	Muzzle->SetArrowColor(FColor::Cyan);
	Muzzle->SetArrowSize(1.5f);

	EnergyBallClass = AEnergyBall::StaticClass();
}

void AEnergyBallEmitter::BeginPlay()
{
	Super::BeginPlay();

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

void AEnergyBallEmitter::StartFiring()
{
	if (!HasAuthority() || bIsFiring) return;

	bIsFiring = true;

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
