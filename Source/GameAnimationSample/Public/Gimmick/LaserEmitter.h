// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaserEmitter.generated.h"

class ALaserReceiver;
class UArrowComponent;
class UBoxComponent;
class UDamageType;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLaserEnabledChanged, bool, bIsEnabled);

/**
 * Continuously traces and renders a damaging laser that can pass through linked Portals.
 */
UCLASS(BlueprintType, Blueprintable, Placeable, meta = (DisplayName = "Laser Emitter"))
class GAMEANIMATIONSAMPLE_API ALaserEmitter : public AActor
{
	GENERATED_BODY()

public:
	ALaserEmitter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Laser")
	void SetLaserEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Laser")
	bool IsLaserEnabled() const { return bLaserEnabled; }

	UPROPERTY(BlueprintAssignable, Category = "Game Animation Sample|Laser")
	FLaserEnabledChanged OnLaserEnabledChanged;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void UpdateLaser();
	void UpdateReceiverContact(ALaserReceiver* NewReceiver);
	void UpdateRedirectorContacts(const TSet<TWeakObjectPtr<AActor>>& NewRedirectors);
	void ReleaseReceiverContact();
	void ReleaseRedirectorContacts();
	void ReleaseAllLaserContacts();
	void ApplyDamageToFinalHit(const FHitResult& FinalHit, const FVector& BeamDirection, float ElapsedSeconds);

	void ApplyVisualAsset();
	void InitializeVisualMaterials();
	void ApplyEmitterVisualState();
	void SetBeamSegment(int32 SegmentIndex, const FVector& Start, const FVector& End);
	bool GetOrCreateBeamSegment(int32 SegmentIndex);
	void HideUnusedBeamSegments(int32 UsedSegmentCount);
	void SetImpactVisual(bool bVisible, const FVector& WorldLocation = FVector::ZeroVector);
	void HideLaserVisuals();

	UFUNCTION()
	void OnRep_LaserEnabled();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** Stable gameplay collision kept independent from the authored visual mesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> EmitterCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> EmitterBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> MuzzleLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ImpactGlow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> ImpactLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Operation", meta = (AllowPrivateAccess = "true"))
	bool bStartEnabled = true;

	UPROPERTY(ReplicatedUsing = OnRep_LaserEnabled, VisibleInstanceOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Operation", meta = (AllowPrivateAccess = "true"))
	bool bLaserEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Operation", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float TraceDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Operation", meta = (AllowPrivateAccess = "true", ClampMin = "0.008333", UIMin = "0.008333", Units = "s"))
	float UpdateInterval = 0.033333f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Portal", meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0"))
	int32 MaxPortalDepth = 4;

	/** Maximum number of redirectors a single beam path may use before terminating. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector", meta = (AllowPrivateAccess = "true", ClampMin = "0", UIMin = "0"))
	int32 MaxRedirectDepth = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Portal", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float PortalExitOffset = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Collision", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel3;

	/** Set to zero to disable damage. Damage is applied by the server only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0"))
	float DamagePerSecond = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Damage", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> DamageTypeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	FLinearColor LaserColor = FLinearColor(1.0f, 0.005f, 0.001f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	FLinearColor CoreColor = FLinearColor(1.0f, 0.82f, 0.72f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	FLinearColor InactiveOpticColor = FLinearColor(0.04f, 0.09f, 0.12f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true", ClampMin = "0.05", UIMin = "0.05", Units = "cm"))
	float BeamCoreRadius = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", UIMin = "0.1", Units = "cm"))
	float BeamGlowRadius = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> LaserVisualMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> ShellMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> MechanismMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> OpticMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> EmissiveMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> EmissiveTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EmitterOpticMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ImpactMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BeamCoreSegments;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BeamGlowSegments;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BeamCoreMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BeamGlowMaterials;

	TWeakObjectPtr<ALaserReceiver> CurrentReceiver;
	TSet<TWeakObjectPtr<AActor>> CurrentRedirectors;
	FTimerHandle LaserUpdateTimer;
	double LastLaserUpdateSeconds = 0.0;
};
