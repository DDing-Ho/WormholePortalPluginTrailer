// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/EnergyReceiverBase.h"
#include "LaserReceiver.generated.h"

class ALaserEmitter;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A laser target that stays active while at least one Laser Emitter is hitting it.
 */
UCLASS(BlueprintType, Blueprintable, Placeable, meta = (DisplayName = "Laser Receiver"))
class GAMEANIMATIONSAMPLE_API ALaserReceiver : public AEnergyReceiverBase
{
	GENERATED_BODY()

public:
	ALaserReceiver();

	/** Adds or removes an emitter from the set currently powering this receiver. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Laser")
	void SetLaserContact(ALaserEmitter* LaserEmitter, bool bInContact);

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Laser")
	int32 GetActiveLaserCount() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleReceiverStateChanged(bool bNewActive) override;

private:
	void RefreshContactState();
	void ApplyVisualAsset();
	void InitializeVisualMaterials();
	void UpdateReceiverVisuals(bool bActive);

	/** Dedicated surface that blocks only the Laser trace channel. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TargetSurface;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ReceiverBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> ReceiverLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	FLinearColor InactiveColor = FLinearColor(0.025f, 0.055f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	FLinearColor ActiveColor = FLinearColor(1.0f, 0.015f, 0.002f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> LaserVisualMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> ShellMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> MechanismMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> OpticMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ReceiverOpticMaterial;

	TSet<TWeakObjectPtr<ALaserEmitter>> ActiveEmitters;
};
