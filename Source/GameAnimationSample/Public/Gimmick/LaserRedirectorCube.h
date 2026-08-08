// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gimmick/LaserRedirector.h"
#include "LaserRedirectorCube.generated.h"

class ALaserEmitter;
class UArrowComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWPTransitComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLaserRedirectingChanged, bool, bIsRedirecting);

/**
 * Movable Portal-style cube that catches a laser on any face and emits it along local +X.
 */
UCLASS(BlueprintType, Blueprintable, Placeable, meta = (DisplayName = "Laser Redirector Cube"))
class GAMEANIMATIONSAMPLE_API ALaserRedirectorCube : public AActor, public ILaserRedirector
{
	GENERATED_BODY()

public:
	ALaserRedirectorCube();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool ResolveLaserRedirect_Implementation(
		const FHitResult& IncomingHit,
		FVector& OutStart,
		FVector& OutDirection) override;
	virtual void SetLaserRedirectContact_Implementation(ALaserEmitter* LaserEmitter, bool bInContact) override;

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Laser|Redirector")
	bool IsRedirectingLaser() const { return bIsRedirecting; }

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Laser|Redirector")
	int32 GetActiveLaserCount() const;

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Laser|Redirector")
	FVector GetLaserOutputLocation() const;

	UFUNCTION(BlueprintPure, Category = "Game Animation Sample|Laser|Redirector")
	FVector GetLaserOutputDirection() const;

	UPROPERTY(BlueprintAssignable, Category = "Game Animation Sample|Laser|Redirector")
	FLaserRedirectingChanged OnRedirectingChanged;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RefreshRedirectingState();
	void ApplyVisualAsset();
	void InitializeVisualMaterials();
	void UpdateRedirectorVisuals();

	UFUNCTION()
	void OnRep_IsRedirecting();

	/** Simulating PhysicsBody grabbed by the player and inspected by Portal Transit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CubeMesh;

	/** Project-owned art mesh; collision and physics stay on the hidden CubeMesh root. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> RedirectorVisual;

	/** Local +X output axis. Its origin lies on the front surface of the cube. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> LaserOutput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> OutputLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWPTransitComponent> TransitComponent;

	/** Distance beyond the front face used to avoid tracing into this cube again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", UIMin = "0.1", Units = "cm"))
	float RedirectionExitOffset = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector|Appearance", meta = (AllowPrivateAccess = "true"))
	FLinearColor InactiveColor = FLinearColor(0.04f, 0.09f, 0.12f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector|Appearance", meta = (AllowPrivateAccess = "true"))
	FLinearColor ActiveColor = FLinearColor(1.0f, 0.01f, 0.001f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> LaserVisualMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> ShellMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> MechanismMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> OpticMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_IsRedirecting, VisibleInstanceOnly, BlueprintReadOnly, Category = "Game Animation Sample|Laser|Redirector", meta = (AllowPrivateAccess = "true"))
	bool bIsRedirecting = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RedirectorOpticMaterial;

	TSet<TWeakObjectPtr<ALaserEmitter>> ActiveEmitters;
};
