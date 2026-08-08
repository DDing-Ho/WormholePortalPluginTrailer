// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnergyBallEmitter.generated.h"

class AEnergyBall;
class UArrowComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnergyBallFired, AEnergyBall*, EnergyBall);

/** Repeatedly or manually fires Portal-compatible Energy Balls. */
UCLASS(BlueprintType, Blueprintable, Placeable, meta = (DisplayName = "Energy Ball Emitter"))
class GAMEANIMATIONSAMPLE_API AEnergyBallEmitter : public AActor
{
	GENERATED_BODY()

public:
	AEnergyBallEmitter();

	/** Starts periodic firing on the authority. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Energy Ball")
	void StartFiring();

	/** Stops periodic firing. Existing Balls remain alive. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Energy Ball")
	void StopFiring();

	/** Fires one Ball immediately, subject to Max Active Balls. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Energy Ball")
	AEnergyBall* FireEnergyBall();

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Energy Ball")
	bool IsFiring() const { return bIsFiring; }

	UPROPERTY(BlueprintAssignable, Category = "Game Animation Sample|Energy Ball")
	FEnergyBallFired OnEnergyBallFired;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void HandleFireTimer();
	void PruneInactiveBalls();
	void ApplyVisualAsset();
	void InitializeVisualMaterials();
	void UpdateEmitterVisuals(bool bActive);

	UFUNCTION()
	void OnRep_IsFiring();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** Stable gameplay collision kept independent from the authored visual mesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> EmitterCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> EmitterMesh;

	/** Defines the exact spawn location and firing direction. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> EmitterLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Firing", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEnergyBall> EnergyBallClass;

	/** Non-positive values use the Ball class's Speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Firing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0", Units = "cm/s"))
	float BallSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Firing", meta = (AllowPrivateAccess = "true", ClampMin = "0.05", UIMin = "0.05", Units = "s"))
	float FireInterval = 2.0f;

	/** Zero allows an unlimited number of live Balls. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Firing", meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0"))
	int32 MaxActiveBalls = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Firing", meta = (AllowPrivateAccess = "true"))
	bool bStartActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Firing", meta = (AllowPrivateAccess = "true", EditCondition = "bStartActive"))
	bool bFireImmediately = true;

	UPROPERTY(ReplicatedUsing = OnRep_IsFiring, VisibleInstanceOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Firing", meta = (AllowPrivateAccess = "true"))
	bool bIsFiring = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> EnergyBallVisualMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> ShellMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> MechanismMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> OpticMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Appearance", meta = (AllowPrivateAccess = "true", HideAlphaChannel))
	FLinearColor InactiveOpticColor = FLinearColor(0.035f, 0.075f, 0.10f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Appearance", meta = (AllowPrivateAccess = "true", HideAlphaChannel))
	FLinearColor ActiveOpticColor = FLinearColor(1.0f, 0.055f, 0.003f, 1.0f);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EmitterOpticMaterial;

	FTimerHandle FireTimer;
	TArray<TWeakObjectPtr<AEnergyBall>> ActiveBalls;
};
