// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SampleChestActor.h"
#include "SampleCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"

USampleChestConfig::USampleChestConfig()
    : Closed(nullptr)
    , Opened(nullptr)
{}

FName ASampleChestActor::ClosedSpriteComponentName(TEXT("Closed"));
FName ASampleChestActor::OpenedSpriteComponentName(TEXT("Opened"));

ASampleChestActor::ASampleChestActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bIsOpened(false)
    , ChestConfig(nullptr)
{
    ClosedComponent = ObjectInitializer.CreateDefaultSubobject<UPaperSpriteComponent>(this, ClosedSpriteComponentName);
    ClosedComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ClosedComponent->SetIsReplicated(true);
    ClosedComponent->SetupAttachment(RootComponent);

    OpenedComponent = ObjectInitializer.CreateDefaultSubobject<UPaperSpriteComponent>(this, OpenedSpriteComponentName);
    OpenedComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OpenedComponent->SetIsReplicated(true);
    OpenedComponent->SetupAttachment(RootComponent);
    bReplicates = true;
}

void ASampleChestActor::PostInitializeComponents()
{
    RefreshConfig();

    Super::PostInitializeComponents();
}

#if WITH_EDITOR
void ASampleChestActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    FProperty* PropertyThatChanged = PropertyChangedEvent.Property;
    if (PropertyThatChanged)
    {
        if (PropertyThatChanged->GetFName() == TEXT("ChestConfig"))
        {
            RefreshConfig();
        }
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void ASampleChestActor::BeginPlay()
{
    Super::BeginPlay();

    SetOpened(bIsOpened);
}

void ASampleChestActor::Interact_Implementation(class AActor* Other)
{
    if (!bIsOpened)
    {
        SetOpened(true);

        Super::Interact_Implementation(Other);
    }
}

void ASampleChestActor::RefreshConfig()
{
    if (ClosedComponent && OpenedComponent)
    {
        if (ChestConfig)
        {
            ClosedComponent->SetSprite(ChestConfig->Closed);
            OpenedComponent->SetSprite(ChestConfig->Opened);
        }
        else
        {
            ClosedComponent->SetSprite(nullptr);
            OpenedComponent->SetSprite(nullptr);
        }
    }
}

void ASampleChestActor::SetOpened(bool bState)
{
    bIsOpened = bState;

    if (ClosedComponent)
    {
        ClosedComponent->SetVisibility(!bState, true);
    }
    if (OpenedComponent)
    {
        OpenedComponent->SetVisibility(bState, true);
    }
}

void ASampleChestActor::Reset()
{
    Super::Reset();

    SetOpened(false);
}