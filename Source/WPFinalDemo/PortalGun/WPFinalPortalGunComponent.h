// Copyright 2026 Team Beaver. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WPFinalPortalGunComponent.generated.h"

class AWormholePortalActor;

UENUM(BlueprintType)
enum class EWPFinalPortalSpawnDirection : uint8
{
	PositiveX UMETA(DisplayName = "+X"),
	NegativeX UMETA(DisplayName = "-X"),
	PositiveY UMETA(DisplayName = "+Y"),
	NegativeY UMETA(DisplayName = "-Y"),
	PositiveZ UMETA(DisplayName = "+Z"),
	NegativeZ UMETA(DisplayName = "-Z"),
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EWPFinalPortalGrowthMode : uint8
{
	VisualOnly UMETA(DisplayName = "Visual Only (Recommended)"),
	PhysicalMetric UMETA(DisplayName = "Physical Metric")
};

/** 충돌 결과를 받아 두 포탈을 만들고 연결하는 독립 컴포넌트. */
UCLASS(ClassGroup = (WormholePortal), BlueprintType, meta = (BlueprintSpawnableComponent))
class WPFINALDEMO_API UWPFinalPortalGunComponent : public UActorComponent
{
	GENERATED_BODY()

  public:
	UWPFinalPortalGunComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Wormhole Portal|Portal Gun")
	bool PlacePortalA(const FHitResult& HitResult);

	UFUNCTION(BlueprintCallable, Category = "Wormhole Portal|Portal Gun")
	bool PlacePortalB(const FHitResult& HitResult);

	UFUNCTION(BlueprintCallable, Category = "Wormhole Portal|Portal Gun")
	void ClearPortals();

	UFUNCTION(BlueprintPure, Category = "Wormhole Portal|Portal Gun")
	bool HasPortalA() const;

	UFUNCTION(BlueprintPure, Category = "Wormhole Portal|Portal Gun")
	bool HasPortalB() const;

	UFUNCTION(BlueprintCallable, Category = "Wormhole Portal|Portal Gun|Spawn")
	void CyclePortalSpawnDirection(float WheelDelta);

  private:
	bool PlacePortal(const FHitResult& HitResult, TObjectPtr<AWormholePortalActor>& PortalSlot, float& GrowthElapsed, bool& bIsGrowing);
	void LinkPortals();
	void UpdateGrowth(float DeltaSeconds);
	void UpdateOnePortalGrowth(AWormholePortalActor* Portal, float DeltaSeconds, float& ElapsedSeconds, bool& bIsGrowing);
	void InitializeFinalPhysicalMetric(AWormholePortalActor* Portal) const;
	void ApplyGrowthScale(AWormholePortalActor* Portal, float Scale) const;
	void ReleasePortal(TObjectPtr<AWormholePortalActor>& Portal, bool& bIsGrowing);
	static float EaseGrowth(float LinearAlpha);
	static FVector ToWorldVector(EWPFinalPortalSpawnDirection Direction);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wormhole Portal|Portal Gun|Spawn", meta = (AllowPrivateAccess = "true"))
	bool bCanSpawnPortalA = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wormhole Portal|Portal Gun|Spawn", meta = (AllowPrivateAccess = "true"))
	bool bCanSpawnPortalB = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wormhole Portal|Portal Gun|Spawn", meta = (AllowPrivateAccess = "true"))
	EWPFinalPortalSpawnDirection PortalSpawnDirection = EWPFinalPortalSpawnDirection::PositiveX;

	UPROPERTY(
		EditAnywhere, BlueprintReadOnly, Category = "Wormhole Portal|Portal Gun|Metric", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float PortalRadius = 300.0f;

	UPROPERTY(
		EditAnywhere, BlueprintReadOnly, Category = "Wormhole Portal|Portal Gun|Metric", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float ThroatHalfLength = 100.0f;

	UPROPERTY(
		EditAnywhere, BlueprintReadOnly, Category = "Wormhole Portal|Portal Gun|Metric", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float TransitionLength = 200.0f;

	UPROPERTY(
		EditAnywhere, BlueprintReadOnly, Category = "Wormhole Portal|Portal Gun|Growth", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float GrowthDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wormhole Portal|Portal Gun|Growth", meta = (AllowPrivateAccess = "true"))
	EWPFinalPortalGrowthMode GrowthMode = EWPFinalPortalGrowthMode::VisualOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wormhole Portal|Portal Gun|Growth",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ClampMax = "1.0"))
	float InitialGrowthScale = 0.05f;

	UPROPERTY(Transient)
	TObjectPtr<AWormholePortalActor> BluePortal;

	UPROPERTY(Transient)
	TObjectPtr<AWormholePortalActor> OrangePortal;

	bool bBluePortalIsGrowing = false;
	bool bOrangePortalIsGrowing = false;
	float BluePortalGrowthSeconds = 0.0f;
	float OrangePortalGrowthSeconds = 0.0f;
};
