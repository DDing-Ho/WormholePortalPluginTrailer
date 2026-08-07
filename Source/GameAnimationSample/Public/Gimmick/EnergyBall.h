// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnergyBall.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UProjectileMovementComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTexture;
class UWPTransitComponent;

/**
 * Portal-compatible Energy Ball projectile.
 *
 * The Ball is moved by Projectile Movement and explicitly opts into Wormhole Portal
 * Projectile Transit. A Receiver consumes the authoritative Ball after a valid overlap.
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Energy Ball"))
class GAMEANIMATIONSAMPLE_API AEnergyBall : public AActor
{
	GENERATED_BODY()

public:
	AEnergyBall();

	/** Launches the Ball in WorldDirection. A non-positive OverrideSpeed uses Speed. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Energy Ball")
	void Launch(const FVector& WorldDirection, float OverrideSpeed = 0.0f);

	/** Consumes the Ball and safely terminates any active Portal Transit. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Animation Sample|Energy Ball")
	void Consume(AActor* ConsumedBy);

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Energy Ball")
	bool IsConsumed() const { return bConsumed; }

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Energy Ball")
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Called immediately before a Receiver destroys the Ball. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Animation Sample|Energy Ball", meta = (DisplayName = "On Consumed"))
	void OnConsumed(AActor* ConsumedBy);

	/** Called before the Ball is destroyed after stopping on a blocking surface. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Animation Sample|Energy Ball", meta = (DisplayName = "On Impacted"))
	void OnImpacted(const FHitResult& Hit);

private:
	void ApplyMovementSettings();
	void InitializeVisualMaterials();
	void UpdateEnergyVisuals(float DeltaSeconds);

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& Hit);

	/** Collision Primitive moved by Projectile Movement and inspected by Portal Transit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Visual-only root rotated independently from Projectile Movement. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> EnergyVisualRoot;

	/** White-hot center of the Energy Ball. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	/** Pulsing inner plasma layer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PlasmaShellMesh;

	/** Large, low-intensity additive halo around the projectile. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> HaloMesh;

	/** Orbiting emissive streaks that simulate unstable electrical arcs. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStaticMeshComponent>> EnergyArcs;

	/** Attached additive afterimages that form a compact comet-like trail. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStaticMeshComponent>> TrailWisps;

	/** Default glow; material/VFX can be replaced in a Blueprint subclass. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> BallLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Enables Projectile Transit through Wormhole Portal Actors. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWPTransitComponent> TransitComponent;

	/** Engine additive material used by every visual layer. Can be replaced in a Blueprint subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> EmissiveMaterial;

	/** White texture supplied to the Engine additive material. */
	UPROPERTY(Transient)
	TObjectPtr<UTexture> EmissiveTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true", HideAlphaChannel))
	FLinearColor EnergyColor = FLinearColor(1.0f, 0.08f, 0.005f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true", HideAlphaChannel))
	FLinearColor CoreColor = FLinearColor(1.0f, 0.72f, 0.24f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0"))
	float CoreEmissiveStrength = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0"))
	float PlasmaEmissiveStrength = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0"))
	float HaloEmissiveStrength = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0", Units = "cd"))
	float LightIntensity = 6500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0"))
	float PulseSpeed = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0"))
	float ArcRotationSpeed = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", UIMin = "1.0", Units = "cm/s"))
	float Speed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0"))
	float GravityScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Movement", meta = (AllowPrivateAccess = "true"))
	bool bBounce = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Movement", meta = (AllowPrivateAccess = "true", EditCondition = "bBounce", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Bounciness = 0.6f;

	/** When true, a Ball that stops on a blocking surface is destroyed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Lifetime", meta = (AllowPrivateAccess = "true"))
	bool bDestroyOnImpact = true;

	/** Zero keeps the Ball alive until impact or consumption. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Energy Ball|Lifetime", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float MaxLifeSeconds = 10.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PlasmaMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HaloMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ArcMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TrailMaterials;

	float VisualTime = 0.0f;
	bool bConsumed = false;
};
