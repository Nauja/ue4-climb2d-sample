// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SamplePlayerController.generated.h"

/**
 * The purpose of this class is to execute the command showflag.postprocessing 0
 * when the player spawns.
 */
UCLASS(config = Game)
class ASamplePlayerController : public APlayerController
{
public:
	GENERATED_BODY()

	virtual void BeginPlay() override;
};