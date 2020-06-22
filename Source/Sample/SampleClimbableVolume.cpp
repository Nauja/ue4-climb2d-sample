// Copyright Epic Games, Inc. All Rights Reserved.

#include "SampleClimbableVolume.h"
#include "Components/BoxComponent.h"
#include "SampleCharacter.h"

ASampleClimbableVolume::ASampleClimbableVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxComponent->InitBoxExtent(FVector(16.0f, 16.0f, 16.0f));
	BoxComponent->SetupAttachment(RootComponent);
}

void ASampleClimbableVolume::NotifyActorBeginOverlap(class AActor* Other)
{
    Super::NotifyActorBeginOverlap(Other);

    if (IsValid(Other) && !IsPendingKill())
    {
        StaticCast<ASampleCharacter*>(Other)->SetClimbEnabled(true);
    }
}

void ASampleClimbableVolume::NotifyActorEndOverlap(class AActor* Other)
{
    Super::NotifyActorEndOverlap(Other);

    if (IsValid(Other) && !IsPendingKill())
    {
        StaticCast<ASampleCharacter*>(Other)->SetClimbEnabled(false);
    }
}