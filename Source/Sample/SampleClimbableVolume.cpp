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

    if (IsValid(Other) && IsValid(this))
    {
        StaticCast<ASampleCharacter*>(Other)->AddClimbableVolume(this);
    }
}

void ASampleClimbableVolume::NotifyActorEndOverlap(class AActor* Other)
{
    Super::NotifyActorEndOverlap(Other);

    if (IsValid(Other) && IsValid(this))
    {
        StaticCast<ASampleCharacter*>(Other)->RemoveClimbableVolume(this);
    }
}