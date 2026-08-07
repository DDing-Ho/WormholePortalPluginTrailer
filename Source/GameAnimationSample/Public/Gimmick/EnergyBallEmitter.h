// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnergyBallEmitter.generated.h"

class AEnergyBall;
class UArrowComponent;
class USceneComponent;
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
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleFireTimer();
	void PruneInactiveBalls();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> EmitterMesh;

	/** Defines the exact spawn location and firing direction. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> Muzzle;

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

	FTimerHandle FireTimer;
	TArray<TWeakObjectPtr<AEnergyBall>> ActiveBalls;
	bool bIsFiring = false;
};
