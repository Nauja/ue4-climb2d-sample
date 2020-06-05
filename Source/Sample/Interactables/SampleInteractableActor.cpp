// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SampleInteractableActor.h"
#include "SampleCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

USampleInteractableConfig::USampleInteractableConfig()
{}

ASampleInteractableActor::ASampleInteractableActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Config(nullptr)
    , bIsEnabled(true)
{
    PrimaryActorTick.bCanEverTick = true;

    SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent"));
    SceneComponent->SetIsReplicated(true);

    RootComponent = SceneComponent;
    bReplicates = true;
}

void ASampleInteractableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASampleInteractableActor, bIsEnabled);
}

void ASampleInteractableActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();
}

#if WITH_EDITOR
/**
 * Update the AActor when assigned USampleInteractableActorConfig changes.
 */
void ASampleInteractableActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    FProperty* PropertyThatChanged = PropertyChangedEvent.Property;
    if (PropertyThatChanged)
    {
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void ASampleInteractableActor::NotifyActorBeginOverlap(class AActor* Other)
{
    Super::NotifyActorBeginOverlap(Other);

    if (IsValid(Other) && !IsPendingKill() && bIsEnabled)
    {
        if (OverlappingActors.Num() == 0)
        {
            NotifyBeginInteractable();
        }
        OverlappingActors.Add(Other);
    }
}

void ASampleInteractableActor::NotifyActorEndOverlap(class AActor* Other)
{
    Super::NotifyActorEndOverlap(Other);

    if (IsValid(Other) && !IsPendingKill() && bIsEnabled)
    {
        OverlappingActors.Remove(Other);
        if (OverlappingActors.Num() == 0)
        {
            NotifyEndInteractable();
        }
    }
}

void ASampleInteractableActor::NotifyBeginInteractable_Implementation()
{
    OnBeginInteractable.Broadcast();
}

void ASampleInteractableActor::NotifyEndInteractable_Implementation()
{
    OnEndInteractable.Broadcast();
}

void ASampleInteractableActor::Interact_Implementation(class AActor* Other)
{
    if (Config)
    {
        /** This will be called after x seconds */
        GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &ASampleInteractableActor::Reset, Config->ResetTimer, false);
    }

    SetIsEnabled(false);
}

void ASampleInteractableActor::SetIsEnabled_Implementation(bool State)
{
    bIsEnabled = State;

    // If re-enabled, check currently overlapping actors
    if (bIsEnabled)
    {
        OverlappingActors.Empty();
        GetOverlappingActors(OverlappingActors, TSubclassOf<ASampleCharacter>());
        if (OverlappingActors.Num() == 0)
        {
            NotifyEndInteractable();
        }
        else
        {
            NotifyBeginInteractable();
        }
    }
    else
    {
        NotifyEndInteractable();
    }
}

void ASampleInteractableActor::OnRep_IsEnabled()
{
    SetIsEnabled(bIsEnabled);
}

void ASampleInteractableActor::Reset()
{
    SetIsEnabled(true);
}
