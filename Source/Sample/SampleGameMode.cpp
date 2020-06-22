// Copyright Epic Games, Inc. All Rights Reserved.

#include "SampleGameMode.h"
#include "SampleCharacter.h"
#include "Kismet/GameplayStatics.h"

ASampleGameMode::ASampleGameMode()
{
	// Set default pawn class to our character
	DefaultPawnClass = ASampleCharacter::StaticClass();	
}

void ASampleGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Allow to keep the true colors of sprites
	APlayerController* Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (Controller)
	{
		Controller->ConsoleCommand(TEXT("showflag.postprocessing 0"));
		Controller->ConsoleCommand(TEXT("r.SetRes 512x448w"));
	}
}
