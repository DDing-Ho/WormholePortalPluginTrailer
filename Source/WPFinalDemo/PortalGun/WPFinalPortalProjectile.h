// Copyright 2026 Team Beaver. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WPFinalPortalProjectile.generated.h"

class UPointLightComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UWPFinalPortalGunComponent;

/** 실제로 날아가 충돌 지점에 포탈을 만드는 발사체. */
UCLASS(Blueprintable)
class WPFINALDEMO_API AWPFinalPortalProjectile : public AActor
{
	GENERATED_BODY()

  public:
	AWPFinalPortalProjectile();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Wormhole Portal|Portal Projectile")
	void InitializeProjectile(UWPFinalPortalGunComponent* InPortalGun, bool bBluePortal);

  protected:
	virtual void BeginPlay() override;

	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal,
		FVector NormalImpulse, const FHitResult& Hit) override;

  private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BlurMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStaticMeshComponent>> TrailSegmentMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> ProjectileLight;

	TWeakObjectPtr<UWPFinalPortalGunComponent> PortalGunComponent;
	FVector SpawnOrigin = FVector::ZeroVector;
	bool bCreatesBluePortal = true;
	bool bHasImpacted = false;
};
