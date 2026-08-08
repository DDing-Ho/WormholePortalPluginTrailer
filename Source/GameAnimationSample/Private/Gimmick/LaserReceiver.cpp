// Copyright 2026 GameAnimationSample. All Rights Reserved.

#include "Gimmick/LaserReceiver.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Gimmick/LaserEmitter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 LaserReceiverRingSegmentCount = 16;
	const FName ColorParameterName(TEXT("Color"));
	const FName TextureParameterName(TEXT("Texture"));

	void ConfigureReceiverEnergyMesh(UStaticMeshComponent* Mesh, const int32 SortPriority)
	{
		if (!IsValid(Mesh)) return;

		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(false);
		Mesh->SetReceivesDecals(false);
		Mesh->SetTranslucentSortPriority(SortPriority);
	}

	FLinearColor MakeEmissiveColor(const FLinearColor& Color, const float Strength)
	{
		FLinearColor Result = Color * FMath::Max(Strength, 0.0f);
		Result.A = 1.0f;
		return Result;
	}
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
	ReceiverBody->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	ReceiverBody->SetRelativeScale3D(FVector(1.9f, 1.9f, 0.18f));
	ReceiverBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReceiverBody->SetGenerateOverlapEvents(false);

	ReceiverGlow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReceiverGlow"));
	ReceiverGlow->SetupAttachment(TargetSurface);
	ReceiverGlow->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	ReceiverGlow->SetRelativeScale3D(FVector(1.42f, 1.42f, 0.055f));
	ConfigureReceiverEnergyMesh(ReceiverGlow, 1);

	ReceiverCore = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReceiverCore"));
	ReceiverCore->SetupAttachment(TargetSurface);
	ReceiverCore->SetRelativeScale3D(FVector(0.32f));
	ConfigureReceiverEnergyMesh(ReceiverCore, 3);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ReceiverBody->SetStaticMesh(CylinderMesh.Object);
		ReceiverGlow->SetStaticMesh(CylinderMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		ReceiverCore->SetStaticMesh(SphereMesh.Object);
	}

	for (int32 SegmentIndex = 0; SegmentIndex < LaserReceiverRingSegmentCount; ++SegmentIndex)
	{
		const FName SegmentName(*FString::Printf(TEXT("ReceiverRing_%02d"), SegmentIndex));
		UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(SegmentName);
		Segment->SetupAttachment(TargetSurface);
		ConfigureReceiverEnergyMesh(Segment, 2);

		if (CylinderMesh.Succeeded())
		{
			Segment->SetStaticMesh(CylinderMesh.Object);
		}

		const float Angle = UE_TWO_PI * static_cast<float>(SegmentIndex) / static_cast<float>(LaserReceiverRingSegmentCount);
		const float RingRadius = 82.0f;
		const FVector RingLocation(0.0f, FMath::Cos(Angle) * RingRadius, FMath::Sin(Angle) * RingRadius);
		const FVector RingTangent(0.0f, -FMath::Sin(Angle), FMath::Cos(Angle));
		Segment->SetRelativeLocation(RingLocation);
		Segment->SetRelativeRotation(FRotationMatrix::MakeFromZ(RingTangent).Rotator());
		Segment->SetRelativeScale3D(FVector(0.045f, 0.045f, 0.29f));
		ReceiverRingSegments.Add(Segment);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AdditiveMaterial(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (AdditiveMaterial.Succeeded())
	{
		EmissiveMaterial = AdditiveMaterial.Object;
		ReceiverGlow->SetMaterial(0, EmissiveMaterial);
		ReceiverCore->SetMaterial(0, EmissiveMaterial);
		for (UStaticMeshComponent* Segment : ReceiverRingSegments)
		{
			Segment->SetMaterial(0, EmissiveMaterial);
		}
	}

	static ConstructorHelpers::FObjectFinder<UTexture> WhiteTexture(
		TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	if (WhiteTexture.Succeeded())
	{
		EmissiveTexture = WhiteTexture.Object;
	}

	ReceiverLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ReceiverLight"));
	ReceiverLight->SetupAttachment(TargetSurface);
	ReceiverLight->SetRelativeLocation(FVector(24.0f, 0.0f, 0.0f));
	ReceiverLight->SetAttenuationRadius(280.0f);
	ReceiverLight->SetCastShadows(false);
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
	if (!HasAuthority() || !IsValid(LaserEmitter)) return;

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

	GlowMaterial = CreateMaterial(ReceiverGlow);
	CoreMaterial = CreateMaterial(ReceiverCore);
	RingMaterials.Reset(ReceiverRingSegments.Num());
	for (UStaticMeshComponent* Segment : ReceiverRingSegments)
	{
		RingMaterials.Add(CreateMaterial(Segment));
	}
}

void ALaserReceiver::UpdateReceiverVisuals(const bool bActive)
{
	const FLinearColor StateColor = bActive ? ActiveColor : InactiveColor;
	const float GlowStrength = bActive ? 3.5f : 0.22f;
	const float CoreStrength = bActive ? 8.0f : 0.45f;
	const float RingStrength = bActive ? 5.5f : 0.30f;

	if (IsValid(GlowMaterial))
	{
		GlowMaterial->SetVectorParameterValue(ColorParameterName, MakeEmissiveColor(StateColor, GlowStrength));
	}
	if (IsValid(CoreMaterial))
	{
		CoreMaterial->SetVectorParameterValue(ColorParameterName, MakeEmissiveColor(StateColor, CoreStrength));
	}
	for (UMaterialInstanceDynamic* RingMaterial : RingMaterials)
	{
		if (IsValid(RingMaterial))
		{
			RingMaterial->SetVectorParameterValue(ColorParameterName, MakeEmissiveColor(StateColor, RingStrength));
		}
	}

	if (IsValid(ReceiverLight))
	{
		ReceiverLight->SetLightColor(StateColor);
		ReceiverLight->SetIntensity(bActive ? 3200.0f : 70.0f);
	}
}
