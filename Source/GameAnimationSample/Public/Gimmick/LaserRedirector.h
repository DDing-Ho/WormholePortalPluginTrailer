// Copyright 2026 GameAnimationSample. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LaserRedirector.generated.h"

class ALaserEmitter;

/** Gameplay contract for Actors that can redirect a Laser Emitter path. */
UINTERFACE(BlueprintType, meta = (DisplayName = "Laser Redirector"))
class GAMEANIMATIONSAMPLE_API ULaserRedirector : public UInterface
{
	GENERATED_BODY()
};

class GAMEANIMATIONSAMPLE_API ILaserRedirector
{
	GENERATED_BODY()

public:
	/** Resolves the World-space start and direction of the redirected beam. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Animation Sample|Laser|Redirector")
	bool ResolveLaserRedirect(const FHitResult& IncomingHit, FVector& OutStart, FVector& OutDirection);

	/** Adds or removes one Laser Emitter from this redirector's active contacts. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Animation Sample|Laser|Redirector")
	void SetLaserRedirectContact(ALaserEmitter* LaserEmitter, bool bInContact);
};
