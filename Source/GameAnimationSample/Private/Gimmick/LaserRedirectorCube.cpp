// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/LaserRedirectorCube.h"

#include "Components/ArrowComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Gimmick/LaserEmitter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Transit/WPTransitComponent.h"
#include "Transit/WPTransitTypes.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName AimWithViewTag(TEXT("AimWithView"));
	const FName EffectColorParameterName(TEXT("EffectColor"));
	const FName EmissiveStrengthParameterName(TEXT("EmissiveStrength"));
	const FName ShellSlotName(TEXT("Shell"));
	const FName MechanismSlotName(TEXT("Mechanism"));
	const FName OpticSlotName(TEXT("Optic"));
}

ALaserRedirectorCube::ALaserRedirectorCube()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	SetRootComponent(CubeMesh);
	CubeMesh->SetMobility(EComponentMobility::Movable);
	CubeMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	CubeMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
	CubeMesh->SetGenerateOverlapEvents(true);
	CubeMesh->SetSimulatePhysics(true);
	CubeMesh->SetLinearDamping(0.35f);
	CubeMesh->SetAngularDamping(1.1f);
	CubeMesh->ComponentTags.Add(AimWithViewTag);
	CubeMesh->SetVisibility(false, false);
	CubeMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		CubeMesh->SetStaticMesh(CubeAsset.Object);
	}

	RedirectorVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RedirectorVisual"));
	RedirectorVisual->SetupAttachment(CubeMesh);
	RedirectorVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RedirectorVisual->SetGenerateOverlapEvents(false);
	RedirectorVisual->SetCastShadow(true);

	LaserOutput = CreateDefaultSubobject<UArrowComponent>(TEXT("LaserOutput"));
	LaserOutput->SetupAttachment(CubeMesh);
	LaserOutput->SetRelativeLocation(FVector(50.0f, 0.0f, 0.0f));
	LaserOutput->SetArrowColor(FColor::Red);
	LaserOutput->SetArrowSize(1.25f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> VisualMeshAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/Laser/Meshes/SM_LaserRedirectorCube.SM_LaserRedirectorCube"));
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

	OutputLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OutputLight"));
	OutputLight->SetupAttachment(LaserOutput);
	OutputLight->SetRelativeLocation(FVector(6.0f, 0.0f, 0.0f));
	OutputLight->SetAttenuationRadius(260.0f);
	OutputLight->SetCastShadows(false);

	TransitComponent = CreateDefaultSubobject<UWPTransitComponent>(TEXT("TransitComponent"));
	TransitComponent->SetTransitType(EWPTransitType::Physics);
}

void ALaserRedirectorCube::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualAsset();
}

void ALaserRedirectorCube::BeginPlay()
{
	Super::BeginPlay();
	InitializeVisualMaterials();
	UpdateRedirectorVisuals();
}

void ALaserRedirectorCube::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveEmitters.Reset();
	bIsRedirecting = false;
	Super::EndPlay(EndPlayReason);
}

void ALaserRedirectorCube::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALaserRedirectorCube, bIsRedirecting);
}

bool ALaserRedirectorCube::ResolveLaserRedirect_Implementation(
	const FHitResult& IncomingHit,
	FVector& OutStart,
	FVector& OutDirection)
{
	OutDirection = GetLaserOutputDirection();
	OutStart = GetLaserOutputLocation() + OutDirection * FMath::Max(RedirectionExitOffset, 0.1f);
	return !OutDirection.IsNearlyZero();
}

void ALaserRedirectorCube::SetLaserRedirectContact_Implementation(ALaserEmitter* LaserEmitter, const bool bInContact)
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
	RefreshRedirectingState();
}

int32 ALaserRedirectorCube::GetActiveLaserCount() const
{
	int32 ValidCount = 0;
	for (const TWeakObjectPtr<ALaserEmitter>& Emitter : ActiveEmitters)
	{
		if (Emitter.IsValid())
		{
			++ValidCount;
		}
	}
	return ValidCount;
}

FVector ALaserRedirectorCube::GetLaserOutputLocation() const
{
	return IsValid(LaserOutput) ? LaserOutput->GetComponentLocation() : GetActorLocation();
}

FVector ALaserRedirectorCube::GetLaserOutputDirection() const
{
	return IsValid(LaserOutput) ? LaserOutput->GetForwardVector().GetSafeNormal() : GetActorForwardVector();
}

void ALaserRedirectorCube::RefreshRedirectingState()
{
	for (auto EmitterIterator = ActiveEmitters.CreateIterator(); EmitterIterator; ++EmitterIterator)
	{
		if (!EmitterIterator->IsValid())
		{
			EmitterIterator.RemoveCurrent();
		}
	}

	const bool bNewRedirecting = !ActiveEmitters.IsEmpty();
	if (bIsRedirecting == bNewRedirecting) return;

	bIsRedirecting = bNewRedirecting;
	UpdateRedirectorVisuals();
	OnRedirectingChanged.Broadcast(bIsRedirecting);
	ForceNetUpdate();
}

void ALaserRedirectorCube::InitializeVisualMaterials()
{
	ApplyVisualAsset();
	RedirectorOpticMaterial = nullptr;
	if (IsValid(RedirectorVisual))
	{
		const int32 OpticMaterialIndex = RedirectorVisual->GetMaterialIndex(OpticSlotName);
		if (OpticMaterialIndex != INDEX_NONE)
		{
			RedirectorOpticMaterial = RedirectorVisual->CreateAndSetMaterialInstanceDynamic(OpticMaterialIndex);
		}
	}
}

void ALaserRedirectorCube::UpdateRedirectorVisuals()
{
	const FLinearColor StateColor = bIsRedirecting ? ActiveColor : InactiveColor;
	if (IsValid(RedirectorOpticMaterial))
	{
		RedirectorOpticMaterial->SetVectorParameterValue(EffectColorParameterName, StateColor);
		RedirectorOpticMaterial->SetScalarParameterValue(
			EmissiveStrengthParameterName,
			bIsRedirecting ? 7.0f : 0.35f);
	}
	if (IsValid(OutputLight))
	{
		OutputLight->SetLightColor(StateColor);
		OutputLight->SetIntensity(bIsRedirecting ? 2800.0f : 60.0f);
	}
}

void ALaserRedirectorCube::ApplyVisualAsset()
{
	if (!IsValid(RedirectorVisual)) return;

	RedirectorVisual->SetStaticMesh(LaserVisualMesh);
	const int32 ShellIndex = RedirectorVisual->GetMaterialIndex(ShellSlotName);
	const int32 MechanismIndex = RedirectorVisual->GetMaterialIndex(MechanismSlotName);
	const int32 OpticIndex = RedirectorVisual->GetMaterialIndex(OpticSlotName);
	if (ShellIndex != INDEX_NONE && IsValid(ShellMaterial))
	{
		RedirectorVisual->SetMaterial(ShellIndex, ShellMaterial);
	}
	if (MechanismIndex != INDEX_NONE && IsValid(MechanismMaterial))
	{
		RedirectorVisual->SetMaterial(MechanismIndex, MechanismMaterial);
	}
	if (OpticIndex != INDEX_NONE && IsValid(OpticMaterial))
	{
		RedirectorVisual->SetMaterial(OpticIndex, OpticMaterial);
	}
}

void ALaserRedirectorCube::OnRep_IsRedirecting()
{
	UpdateRedirectorVisuals();
	OnRedirectingChanged.Broadcast(bIsRedirecting);
}
