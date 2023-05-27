// Copyright Epic Games, Inc. All Rights Reserved.


#include "SamplePlayerController.h"


void ASamplePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Allow to keep the true colors of sprites
	ConsoleCommand(TEXT("showflag.postprocessing 0"));
}