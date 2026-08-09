// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/LaserEmitter.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Gimmick/LaserRedirector.h"
#include "Gimmick/LaserReceiver.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Trace/WPTraceLibrary.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName ColorParameterName(TEXT("Color"));
	const FName TextureParameterName(TEXT("Texture"));
	const FName EffectColorParameterName(TEXT("EffectColor"));
	const FName EmissiveStrengthParameterName(TEXT("EmissiveStrength"));
	const FName ShellSlotName(TEXT("Shell"));
	const FName MechanismSlotName(TEXT("Mechanism"));
	const FName OpticSlotName(TEXT("Optic"));
	constexpr int32 ForegroundTranslucencyStencilValue = 240;

	void ConfigureLaserVisualMesh(UStaticMeshComponent* Mesh, const int32 SortPriority)
	{
		if (!IsValid(Mesh)) return;

		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(false);
		Mesh->SetReceivesDecals(false);
		Mesh->SetTranslucentSortPriority(SortPriority);
		Mesh->SetRenderCustomDepth(true);
		Mesh->SetCustomDepthStencilValue(ForegroundTranslucencyStencilValue);
	}

	FLinearColor MakeEmissiveColor(const FLinearColor& Color, const float Strength)
	{
		FLinearColor Result = Color * FMath::Max(Strength, 0.0f);
		Result.A = 1.0f;
		return Result;
	}

	void SetOpticMaterialState(
		UMaterialInstanceDynamic* Material,
		const FLinearColor& Color,
		const float Strength)
	{
		if (!IsValid(Material)) return;
		Material->SetVectorParameterValue(EffectColorParameterName, Color);
		Material->SetScalarParameterValue(EmissiveStrengthParameterName, FMath::Max(Strength, 0.0f));
	}
}

ALaserEmitter::ALaserEmitter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EmitterCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("EmitterCollision"));
	EmitterCollision->SetupAttachment(SceneRoot);
	EmitterCollision->InitBoxExtent(FVector(38.0f, 57.0f, 57.0f));
	EmitterCollision->SetRelativeLocation(FVector(38.0f, 0.0f, 0.0f));
	EmitterCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	EmitterCollision->SetGenerateOverlapEvents(false);

	EmitterBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterBody"));
	EmitterBody->SetupAttachment(SceneRoot);
	EmitterBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EmitterBody->SetGenerateOverlapEvents(false);
	EmitterBody->SetCastShadow(true);

	Muzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(SceneRoot);
	Muzzle->SetRelativeLocation(FVector(76.0f, 0.0f, 0.0f));
	Muzzle->SetArrowColor(FColor::Red);
	Muzzle->SetArrowSize(1.5f);

	ImpactGlow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImpactGlow"));
	ImpactGlow->SetupAttachment(SceneRoot);
	ImpactGlow->SetRelativeScale3D(FVector(0.14f));
	ConfigureLaserVisualMesh(ImpactGlow, 4);
	ImpactGlow->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderAsset.Succeeded())
	{
		CylinderMesh = CylinderAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereAsset.Succeeded())
	{
		ImpactGlow->SetStaticMesh(SphereAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> VisualMeshAsset(
		TEXT("/Game/GameAnimationSample/Gimmicks/Laser/Meshes/SM_LaserEmitter.SM_LaserEmitter"));
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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AdditiveMaterial(
		TEXT("/Game/GameAnimationSample/Gimmicks/Shared/Materials/M_GimmickEmissiveCustomDepth.M_GimmickEmissiveCustomDepth"));
	if (AdditiveMaterial.Succeeded())
	{
		EmissiveMaterial = AdditiveMaterial.Object;
		ImpactGlow->SetMaterial(0, EmissiveMaterial);
	}

	static ConstructorHelpers::FObjectFinder<UTexture> WhiteTexture(
		TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	if (WhiteTexture.Succeeded())
	{
		EmissiveTexture = WhiteTexture.Object;
	}

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Muzzle);
	MuzzleLight->SetAttenuationRadius(260.0f);
	MuzzleLight->SetCastShadows(false);

	ImpactLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ImpactLight"));
	ImpactLight->SetupAttachment(SceneRoot);
	ImpactLight->SetAttenuationRadius(220.0f);
	ImpactLight->SetCastShadows(false);
	ImpactLight->SetVisibility(false);

	DamageTypeClass = UDamageType::StaticClass();
}

void ALaserEmitter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualAsset();
}

void ALaserEmitter::BeginPlay()
{
	Super::BeginPlay();
	InitializeVisualMaterials();

	if (HasAuthority())
	{
		bLaserEnabled = bStartEnabled;
		ForceNetUpdate();
	}

	if (const UWorld* World = GetWorld())
	{
		LastLaserUpdateSeconds = World->GetTimeSeconds();
	}

	ApplyEmitterVisualState();
	UpdateLaser();
	GetWorldTimerManager().SetTimer(
		LaserUpdateTimer,
		this,
		&ALaserEmitter::UpdateLaser,
		FMath::Max(UpdateInterval, 1.0f / 120.0f),
		true);
}

void ALaserEmitter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LaserUpdateTimer);
	ReleaseAllLaserContacts();
	HideLaserVisuals();
	Super::EndPlay(EndPlayReason);
}

void ALaserEmitter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALaserEmitter, bLaserEnabled);
}

void ALaserEmitter::SetLaserEnabled(const bool bEnabled)
{
	if (!HasAuthority() || bLaserEnabled == bEnabled) return;

	bLaserEnabled = bEnabled;
	if (!bLaserEnabled)
	{
		ReleaseAllLaserContacts();
		HideLaserVisuals();
	}

	ApplyEmitterVisualState();
	OnLaserEnabledChanged.Broadcast(bLaserEnabled);
	ForceNetUpdate();
}

void ALaserEmitter::OnRep_LaserEnabled()
{
	if (!bLaserEnabled)
	{
		HideLaserVisuals();
	}
	ApplyEmitterVisualState();
	OnLaserEnabledChanged.Broadcast(bLaserEnabled);
}

void ALaserEmitter::UpdateLaser()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		ReleaseAllLaserContacts();
		HideLaserVisuals();
		return;
	}

	const double CurrentSeconds = World->GetTimeSeconds();
	const float ElapsedSeconds = static_cast<float>(FMath::Max(CurrentSeconds - LastLaserUpdateSeconds, 0.0));
	LastLaserUpdateSeconds = CurrentSeconds;

	if (!bLaserEnabled || !IsValid(Muzzle))
	{
		ReleaseAllLaserContacts();
		HideLaserVisuals();
		return;
	}

	const FVector TraceStart = Muzzle->GetComponentLocation();
	const FVector InitialDirection = Muzzle->GetForwardVector().GetSafeNormal();
	if (InitialDirection.IsNearlyZero() || TraceDistance <= 0.0f)
	{
		ReleaseAllLaserContacts();
		HideLaserVisuals();
		return;
	}

	const bool bRenderVisuals = World->GetNetMode() != NM_DedicatedServer;
	int32 UsedSegmentCount = 0;
	int32 RemainingPortalDepth = FMath::Max(MaxPortalDepth, 0);
	int32 RedirectDepth = 0;
	float RemainingDistance = FMath::Max(TraceDistance, 0.0f);
	FVector CurrentTraceStart = TraceStart;
	FVector CurrentTraceDirection = InitialDirection;
	TSet<TWeakObjectPtr<AActor>> NewRedirectors;
	TSet<TWeakObjectPtr<AActor>> VisitedRedirectors;

	FHitResult TerminalHit;
	FVector TerminalBeamDirection = InitialDirection;
	FVector TerminalImpactLocation = FVector::ZeroVector;
	bool bHasTerminalHit = false;
	bool bTerminalActorIsRedirector = false;
	bool bShowTerminalImpact = false;
	bool bPathTerminated = false;
	bool bInvalidPath = false;

	while (!bPathTerminated && RemainingDistance > KINDA_SMALL_NUMBER)
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LaserEmitter), false, this);
		if (IsValid(GetOwner()) && GetOwner() != this)
		{
			QueryParams.AddIgnoredActor(GetOwner());
		}
		if (IsValid(GetInstigator()))
		{
			QueryParams.AddIgnoredActor(GetInstigator());
		}
		FWPPortalTraceResult TraceResult;
		const bool bBlockingHit = UWPTraceLibrary::PortalLineTraceSingleByChannel(
			this,
			TraceResult,
			CurrentTraceStart,
			CurrentTraceStart + CurrentTraceDirection * RemainingDistance,
			TraceChannel.GetValue(),
			QueryParams,
			FCollisionResponseParams::DefaultResponseParam,
			RemainingPortalDepth,
			FMath::Max(PortalExitOffset, 0.0f));

		if (TraceResult.Status == EWPPortalTraceStatus::InvalidInput)
		{
			bInvalidPath = true;
			break;
		}

		RemainingPortalDepth = FMath::Max(RemainingPortalDepth - TraceResult.PortalTraversalCount, 0);
		FVector SegmentStart = CurrentTraceStart;
		FVector SegmentDirection = CurrentTraceDirection;
		float SegmentLogicalStart = 0.0f;
		bool bPortalTerminatedPath = false;

		for (const FWPPortalTracePortalEvent& PortalEvent : TraceResult.PortalEvents)
		{
			if (bRenderVisuals)
			{
				SetBeamSegment(UsedSegmentCount++, SegmentStart, PortalEvent.DetectionHit.ImpactPoint);
			}

			if (PortalEvent.Outcome != EWPPortalTracePortalOutcome::Traversed)
			{
				TerminalImpactLocation = PortalEvent.DetectionHit.ImpactPoint;
				bShowTerminalImpact = true;
				bPathTerminated = true;
				bPortalTerminatedPath = true;
				break;
			}

			SegmentStart = PortalEvent.ExitTraceStart;
			SegmentDirection = PortalEvent.ExitDirection.GetSafeNormal();
			SegmentLogicalStart = PortalEvent.LogicalDistance;
		}

		if (bPortalTerminatedPath)
		{
			break;
		}
		if (TraceResult.Status != EWPPortalTraceStatus::Completed)
		{
			bPathTerminated = true;
			break;
		}

		FHitResult FinalHit;
		bool bHasFinalHit = false;
		FVector SegmentEnd;
		if (bBlockingHit && !TraceResult.SceneHits.IsEmpty())
		{
			FinalHit = TraceResult.SceneHits.Last().Hit;
			SegmentEnd = FinalHit.ImpactPoint;
			bHasFinalHit = true;
		}
		else
		{
			const float SegmentRemainingDistance = FMath::Max(RemainingDistance - SegmentLogicalStart, 0.0f);
			SegmentEnd = SegmentStart + SegmentDirection * SegmentRemainingDistance;
		}

		if (bRenderVisuals)
		{
			SetBeamSegment(UsedSegmentCount++, SegmentStart, SegmentEnd);
		}

		const float ConsumedDistance = bHasFinalHit
			? FMath::Clamp(TraceResult.ProcessedDistance, 0.0f, RemainingDistance)
			: RemainingDistance;
		RemainingDistance = FMath::Max(RemainingDistance - ConsumedDistance, 0.0f);

		if (!bHasFinalHit)
		{
			bPathTerminated = true;
			break;
		}

		AActor* HitActor = FinalHit.GetActor();
		const bool bHitRedirector = IsValid(HitActor)
			&& HitActor->GetClass()->ImplementsInterface(ULaserRedirector::StaticClass());
		if (!bHitRedirector)
		{
			TerminalHit = FinalHit;
			TerminalBeamDirection = SegmentDirection;
			TerminalImpactLocation = SegmentEnd;
			bHasTerminalHit = true;
			bShowTerminalImpact = true;
			bPathTerminated = true;
			break;
		}

		const TWeakObjectPtr<AActor> RedirectorKey(HitActor);
		NewRedirectors.Add(RedirectorKey);
		const bool bCycleDetected = VisitedRedirectors.Contains(RedirectorKey);
		const bool bRedirectLimitReached = RedirectDepth >= FMath::Max(MaxRedirectDepth, 0);
		if (bCycleDetected || bRedirectLimitReached)
		{
			TerminalHit = FinalHit;
			TerminalBeamDirection = SegmentDirection;
			TerminalImpactLocation = SegmentEnd;
			bHasTerminalHit = true;
			bTerminalActorIsRedirector = true;
			bShowTerminalImpact = true;
			bPathTerminated = true;
			break;
		}

		FVector RedirectStart = FVector::ZeroVector;
		FVector RedirectDirection = FVector::ZeroVector;
		const bool bResolvedRedirect = ILaserRedirector::Execute_ResolveLaserRedirect(
			HitActor,
			FinalHit,
			RedirectStart,
			RedirectDirection);
		RedirectDirection = RedirectDirection.GetSafeNormal();
		if (!bResolvedRedirect
			|| RedirectStart.ContainsNaN()
			|| RedirectDirection.ContainsNaN()
			|| RedirectDirection.IsNearlyZero())
		{
			TerminalHit = FinalHit;
			TerminalBeamDirection = SegmentDirection;
			TerminalImpactLocation = SegmentEnd;
			bHasTerminalHit = true;
			bTerminalActorIsRedirector = true;
			bShowTerminalImpact = true;
			bPathTerminated = true;
			break;
		}

		VisitedRedirectors.Add(RedirectorKey);
		++RedirectDepth;
		CurrentTraceStart = RedirectStart;
		CurrentTraceDirection = RedirectDirection;
	}

	if (bInvalidPath)
	{
		ReleaseAllLaserContacts();
		HideLaserVisuals();
		return;
	}

	if (bRenderVisuals)
	{
		HideUnusedBeamSegments(UsedSegmentCount);
		SetImpactVisual(bShowTerminalImpact, TerminalImpactLocation);
	}

	if (HasAuthority())
	{
		UpdateRedirectorContacts(NewRedirectors);
		ALaserReceiver* HitReceiver = bHasTerminalHit && !bTerminalActorIsRedirector
			? Cast<ALaserReceiver>(TerminalHit.GetActor())
			: nullptr;
		UpdateReceiverContact(HitReceiver);
		if (bHasTerminalHit && !bTerminalActorIsRedirector && !IsValid(HitReceiver))
		{
			ApplyDamageToFinalHit(TerminalHit, TerminalBeamDirection, ElapsedSeconds);
		}
	}
}

void ALaserEmitter::UpdateReceiverContact(ALaserReceiver* NewReceiver)
{
	if (!HasAuthority() || CurrentReceiver.Get() == NewReceiver) return;

	if (ALaserReceiver* PreviousReceiver = CurrentReceiver.Get())
	{
		PreviousReceiver->SetLaserContact(this, false);
	}

	CurrentReceiver = NewReceiver;
	if (IsValid(NewReceiver))
	{
		NewReceiver->SetLaserContact(this, true);
	}
}

void ALaserEmitter::UpdateRedirectorContacts(const TSet<TWeakObjectPtr<AActor>>& NewRedirectors)
{
	if (!HasAuthority()) return;

	for (const TWeakObjectPtr<AActor>& Redirector : CurrentRedirectors)
	{
		AActor* RedirectorActor = Redirector.Get();
		if (!NewRedirectors.Contains(Redirector)
			&& IsValid(RedirectorActor)
			&& RedirectorActor->GetClass()->ImplementsInterface(ULaserRedirector::StaticClass()))
		{
			ILaserRedirector::Execute_SetLaserRedirectContact(RedirectorActor, this, false);
		}
	}

	for (const TWeakObjectPtr<AActor>& Redirector : NewRedirectors)
	{
		AActor* RedirectorActor = Redirector.Get();
		if (!CurrentRedirectors.Contains(Redirector)
			&& IsValid(RedirectorActor)
			&& RedirectorActor->GetClass()->ImplementsInterface(ULaserRedirector::StaticClass()))
		{
			ILaserRedirector::Execute_SetLaserRedirectContact(RedirectorActor, this, true);
		}
	}

	CurrentRedirectors = NewRedirectors;
}

void ALaserEmitter::ReleaseReceiverContact()
{
	if (!HasAuthority()) return;
	UpdateReceiverContact(nullptr);
}

void ALaserEmitter::ReleaseRedirectorContacts()
{
	if (!HasAuthority()) return;
	UpdateRedirectorContacts(TSet<TWeakObjectPtr<AActor>>());
}

void ALaserEmitter::ReleaseAllLaserContacts()
{
	ReleaseReceiverContact();
	ReleaseRedirectorContacts();
}

void ALaserEmitter::ApplyDamageToFinalHit(
	const FHitResult& FinalHit,
	const FVector& BeamDirection,
	const float ElapsedSeconds)
{
	AActor* HitActor = FinalHit.GetActor();
	if (!HasAuthority()
		|| !IsValid(HitActor)
		|| HitActor == this
		|| !HitActor->CanBeDamaged()
		|| DamagePerSecond <= 0.0f
		|| ElapsedSeconds <= 0.0f)
	{
		return;
	}

	const TSubclassOf<UDamageType> EffectiveDamageType = DamageTypeClass
		? DamageTypeClass
		: TSubclassOf<UDamageType>(UDamageType::StaticClass());

	UGameplayStatics::ApplyPointDamage(
		HitActor,
		DamagePerSecond * ElapsedSeconds,
		BeamDirection,
		FinalHit,
		GetInstigatorController(),
		this,
		EffectiveDamageType);
}

void ALaserEmitter::InitializeVisualMaterials()
{
	ApplyVisualAsset();
	EmitterOpticMaterial = nullptr;
	if (IsValid(EmitterBody))
	{
		const int32 OpticMaterialIndex = EmitterBody->GetMaterialIndex(OpticSlotName);
		if (OpticMaterialIndex != INDEX_NONE)
		{
			EmitterOpticMaterial = EmitterBody->CreateAndSetMaterialInstanceDynamic(OpticMaterialIndex);
		}
	}

	if (!IsValid(EmissiveMaterial)) return;

	auto CreateMaterial = [this](UStaticMeshComponent* Mesh) -> UMaterialInstanceDynamic*
	{
		if (!IsValid(Mesh)) return nullptr;

		UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0);
		if (IsValid(Material) && IsValid(EmissiveTexture))
		{
			Material->SetTextureParameterValue(TextureParameterName, EmissiveTexture);
		}
		return Material;
	};

	ImpactMaterial = CreateMaterial(ImpactGlow);

	if (IsValid(ImpactMaterial))
	{
		ImpactMaterial->SetVectorParameterValue(ColorParameterName, MakeEmissiveColor(LaserColor, 9.0f));
	}
}

void ALaserEmitter::ApplyEmitterVisualState()
{
	SetOpticMaterialState(
		EmitterOpticMaterial,
		bLaserEnabled ? LaserColor : InactiveOpticColor,
		bLaserEnabled ? 7.0f : 0.35f);
	if (IsValid(MuzzleLight))
	{
		MuzzleLight->SetVisibility(true);
		MuzzleLight->SetLightColor(bLaserEnabled ? LaserColor : InactiveOpticColor);
		MuzzleLight->SetIntensity(bLaserEnabled ? 2600.0f : 45.0f);
	}
}

void ALaserEmitter::ApplyVisualAsset()
{
	if (!IsValid(EmitterBody)) return;

	EmitterBody->SetStaticMesh(LaserVisualMesh);
	const int32 ShellIndex = EmitterBody->GetMaterialIndex(ShellSlotName);
	const int32 MechanismIndex = EmitterBody->GetMaterialIndex(MechanismSlotName);
	const int32 OpticIndex = EmitterBody->GetMaterialIndex(OpticSlotName);
	if (ShellIndex != INDEX_NONE && IsValid(ShellMaterial))
	{
		EmitterBody->SetMaterial(ShellIndex, ShellMaterial);
	}
	if (MechanismIndex != INDEX_NONE && IsValid(MechanismMaterial))
	{
		EmitterBody->SetMaterial(MechanismIndex, MechanismMaterial);
	}
	if (OpticIndex != INDEX_NONE && IsValid(OpticMaterial))
	{
		EmitterBody->SetMaterial(OpticIndex, OpticMaterial);
	}
}

void ALaserEmitter::SetBeamSegment(const int32 SegmentIndex, const FVector& Start, const FVector& End)
{
	if (!GetOrCreateBeamSegment(SegmentIndex)) return;

	const FVector Delta = End - Start;
	const float Length = Delta.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		BeamCoreSegments[SegmentIndex]->SetVisibility(false);
		BeamGlowSegments[SegmentIndex]->SetVisibility(false);
		return;
	}

	const FVector Midpoint = Start + Delta * 0.5f;
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Delta / Length).Rotator();

	UStaticMeshComponent* CoreSegment = BeamCoreSegments[SegmentIndex];
	CoreSegment->SetWorldLocation(Midpoint);
	CoreSegment->SetWorldRotation(Rotation);
	CoreSegment->SetWorldScale3D(FVector(
		FMath::Max(BeamCoreRadius, 0.05f) / 50.0f,
		FMath::Max(BeamCoreRadius, 0.05f) / 50.0f,
		Length / 100.0f));
	CoreSegment->SetVisibility(true);

	UStaticMeshComponent* GlowSegment = BeamGlowSegments[SegmentIndex];
	GlowSegment->SetWorldLocation(Midpoint);
	GlowSegment->SetWorldRotation(Rotation);
	GlowSegment->SetWorldScale3D(FVector(
		FMath::Max(BeamGlowRadius, BeamCoreRadius) / 50.0f,
		FMath::Max(BeamGlowRadius, BeamCoreRadius) / 50.0f,
		Length / 100.0f));
	GlowSegment->SetVisibility(true);
}

bool ALaserEmitter::GetOrCreateBeamSegment(const int32 SegmentIndex)
{
	if (SegmentIndex < 0 || !IsValid(CylinderMesh) || !IsValid(EmissiveMaterial)) return false;

	while (BeamCoreSegments.Num() <= SegmentIndex)
	{
		const int32 NewIndex = BeamCoreSegments.Num();
		auto CreateSegment = [this, NewIndex](const TCHAR* LayerName, const int32 SortPriority, const FLinearColor& Color, const float Strength,
			TArray<TObjectPtr<UStaticMeshComponent>>& SegmentArray,
			TArray<TObjectPtr<UMaterialInstanceDynamic>>& MaterialArray) -> bool
		{
			const FName ComponentName(*FString::Printf(TEXT("Laser%sSegment_%02d"), LayerName, NewIndex));
			UStaticMeshComponent* Segment = NewObject<UStaticMeshComponent>(this, ComponentName);
			if (!IsValid(Segment)) return false;

			Segment->SetupAttachment(SceneRoot);
			Segment->SetStaticMesh(CylinderMesh);
			Segment->SetMaterial(0, EmissiveMaterial);
			ConfigureLaserVisualMesh(Segment, SortPriority);
			Segment->SetVisibility(false);
			AddInstanceComponent(Segment);
			Segment->RegisterComponent();

			UMaterialInstanceDynamic* Material = Segment->CreateAndSetMaterialInstanceDynamic(0);
			if (IsValid(Material))
			{
				if (IsValid(EmissiveTexture))
				{
					Material->SetTextureParameterValue(TextureParameterName, EmissiveTexture);
				}
				Material->SetVectorParameterValue(ColorParameterName, MakeEmissiveColor(Color, Strength));
			}

			SegmentArray.Add(Segment);
			MaterialArray.Add(Material);
			return true;
		};

		if (!CreateSegment(TEXT("Glow"), 1, LaserColor, 2.5f, BeamGlowSegments, BeamGlowMaterials))
		{
			return false;
		}
		if (!CreateSegment(TEXT("Core"), 2, CoreColor, 8.0f, BeamCoreSegments, BeamCoreMaterials))
		{
			return false;
		}
	}

	return BeamCoreSegments.IsValidIndex(SegmentIndex) && BeamGlowSegments.IsValidIndex(SegmentIndex);
}

void ALaserEmitter::HideUnusedBeamSegments(const int32 UsedSegmentCount)
{
	for (int32 SegmentIndex = FMath::Max(UsedSegmentCount, 0); SegmentIndex < BeamCoreSegments.Num(); ++SegmentIndex)
	{
		if (IsValid(BeamCoreSegments[SegmentIndex]))
		{
			BeamCoreSegments[SegmentIndex]->SetVisibility(false);
		}
		if (BeamGlowSegments.IsValidIndex(SegmentIndex) && IsValid(BeamGlowSegments[SegmentIndex]))
		{
			BeamGlowSegments[SegmentIndex]->SetVisibility(false);
		}
	}
}

void ALaserEmitter::SetImpactVisual(const bool bVisible, const FVector& WorldLocation)
{
	if (IsValid(ImpactGlow))
	{
		ImpactGlow->SetVisibility(bVisible);
		if (bVisible)
		{
			ImpactGlow->SetWorldLocation(WorldLocation);
		}
	}
	if (IsValid(ImpactLight))
	{
		ImpactLight->SetVisibility(bVisible);
		if (bVisible)
		{
			ImpactLight->SetWorldLocation(WorldLocation);
			ImpactLight->SetLightColor(LaserColor);
			ImpactLight->SetIntensity(2200.0f);
		}
	}
}

void ALaserEmitter::HideLaserVisuals()
{
	HideUnusedBeamSegments(0);
	SetImpactVisual(false);
}
