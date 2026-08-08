// Copyright 2026 Team Beaver. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Variant_Shooter/Weapons/ShooterWeapon.h"
#include "WPFinalPortalGunWeapon.generated.h"

class UAnimMontage;
class UArrowComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USceneComponent;
class UWPFinalPortalGunComponent;

/** FPS 무기 입력을 포탈 발사체와 연결하는 포탈건. */
UCLASS(Blueprintable)
class WPFINALDEMO_API AWPFinalPortalGunWeapon : public AShooterWeapon
{
	GENERATED_BODY()

public:
	AWPFinalPortalGunWeapon();

	virtual void Tick(float DeltaSeconds) override;
	virtual void StartAlternateFiring() override;

protected:
	virtual void BeginPlay() override;
	virtual void FireProjectile(const FVector& TargetLocation) override;

private:
	bool ShootPortal(const FVector& TargetLocation, bool bBluePortal);
	void SpawnMuzzleFlash(const FVector& MuzzleLocation, const FVector& AimDirection, bool bBluePortal);
	void PlayFireAnimation();
	void CreateTubeMaterials();
	void SetTubeColorForShot(bool bBluePortal);
	void ApplyTubeMaterial(const FLinearColor& Color, float EmissiveStrength, float Opacity);
	void InitializeFirstPersonSmoothing();
	void UpdateFirstPersonSmoothing(float DeltaSeconds);
	void StartRecoil();
	void ApplyRecoil(float RecoilAlpha);
	float CalculateRecoilAlpha() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wormhole Portal|Portal Gun", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWPFinalPortalGunComponent> PortalGunComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wormhole Portal|Portal Gun|Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> ProjectileMuzzle;

	UPROPERTY()
	TObjectPtr<USceneComponent> FirstPersonSmoothingPivot;

	UPROPERTY()
	TObjectPtr<USceneComponent> FirstPersonSmoothingTarget;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> TubeMaterial;

	UPROPERTY()
	TObjectPtr<UAnimMontage> FireMontage;

	float LastOrangeShotTime = 0.0f;
	FVector FirstPersonRestLocation = FVector::ZeroVector;
	FVector ThirdPersonRestLocation = FVector::ZeroVector;
	float RecoilElapsedSeconds = 0.0f;
	float RecoilStartAlpha = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FirstPersonTubeDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ThirdPersonTubeDynamicMaterial;

	bool bRecoilActive = false;
};
