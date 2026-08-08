// Copyright 2026 Team Beaver. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WPFinalPortalBurstEffect.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/** 총구 섬광과 충돌 파편을 만드는 짧은 수명의 이펙트 액터. */
UCLASS(Blueprintable)
class WPFINALDEMO_API AWPFinalPortalBurstEffect : public AActor
{
	GENERATED_BODY()

  public:
	AWPFinalPortalBurstEffect();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Wormhole Portal|Portal VFX")
	void InitializeMuzzle(bool bBluePortal);

	UFUNCTION(BlueprintCallable, Category = "Wormhole Portal|Portal VFX")
	void InitializeImpact(bool bBluePortal);

  private:
	void InitializeEffect(bool bBluePortal, bool bImpact);
	void CreateParticles(FRandomStream& RandomStream);
	void UpdateOpacity(float Opacity);

	UPROPERTY(VisibleAnywhere, Category = "Portal VFX")
	TObjectPtr<UStaticMeshComponent> OuterFlashMesh;

	UPROPERTY(VisibleAnywhere, Category = "Portal VFX")
	TObjectPtr<UStaticMeshComponent> InnerCoreMesh;

	UPROPERTY(VisibleAnywhere, Category = "Portal VFX")
	TArray<TObjectPtr<UStaticMeshComponent>> ParticleMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Portal VFX")
	TObjectPtr<UPointLightComponent> EffectLight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OuterFlashMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InnerCoreMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ParticleMaterial;

	TArray<FVector> ParticlePositions;
	TArray<FVector> ParticleVelocities;
	TArray<FVector> ParticleBaseScales;
	float ElapsedSeconds = 0.0f;
	bool bIsImpact = false;
};
