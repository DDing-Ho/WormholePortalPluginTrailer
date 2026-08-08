// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/EnergyReceiverBase.h"
#include "EnergyBallReceiver.generated.h"

class AEnergyBall;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnergyBallReceived, AEnergyBall*, EnergyBall);

/**
 * Absorbs Energy Balls and exposes their arrival as a reusable gameplay trigger.
 *
 * Bind to On Activation Changed, override the Blueprint events, or add Connected Actors
 * that implement Energy Triggerable.
 */
UCLASS(BlueprintType, Blueprintable, Placeable, meta = (DisplayName = "Energy Ball Receiver"))
class GAMEANIMATIONSAMPLE_API AEnergyBallReceiver : public AEnergyReceiverBase
{
	GENERATED_BODY()

public:
	AEnergyBallReceiver();

	/** Validates and receives a Ball. Returns true only when the Ball was accepted. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Energy Ball")
	bool ReceiveEnergyBall(AEnergyBall* EnergyBall);

	/** Manually controls the trigger state. Timed activation is restarted when set true again. */
	virtual void SetReceiverActive(bool bNewActive) override;

	UPROPERTY(BlueprintAssignable, Category = "Game Animation Sample|Energy Ball")
	FEnergyBallReceived OnEnergyBallReceived;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleReceiverStateChanged(bool bNewActive) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Animation Sample|Energy Ball", meta = (DisplayName = "On Ball Received"))
	void OnBallReceived(AEnergyBall* EnergyBall);

private:
	void ScheduleDeactivation();
	void DeactivateAfterDelay();
	void ApplyVisualAsset();
	void InitializeVisualMaterials();
	void UpdateReceiverVisuals(bool bActive);

	UFUNCTION()
	void HandleDetectionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	/** Query-only volume that accepts WorldDynamic Energy Balls. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> DetectionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ReceiverMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> ReceiverLight;

	/** Optional subclass filter. Empty accepts every Energy Ball subclass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Trigger", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEnergyBall> AcceptedBallClass;

	/** Zero latches active until Set Receiver Active(false) is called. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Trigger", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float ActiveDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Trigger", meta = (AllowPrivateAccess = "true"))
	bool bConsumeBall = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Trigger", meta = (AllowPrivateAccess = "true"))
	bool bStartActive = false;

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
	TObjectPtr<UMaterialInstanceDynamic> ReceiverOpticMaterial;

	FTimerHandle DeactivationTimer;
};
