# ue4-climb2d-sample

![UE4](https://img.shields.io/badge/UE4-4.25+-blue)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/Nauja/ue4-jetpack-sample/master/LICENSE)

Sample of a custom climbing movement done with Paper2D.

![Preview](https://github.com/Nauja/ue4-climb2d-sample/raw/master/docs/preview.gif)

This project is an example of how to write a custom climbing movement in a Paper2D game, with the constraint of
being fully replicated over network.

Prerequisites:
  * [Pixel Perfect 2D Sample](https://github.com/Nauja/ue4-pixelperfect2d-sample)

Features:
  * Instructions
  * Detecting when character can climb
  * Switching to climbing movement mode
  * Allowing to jump while climbing

### Instructions

Keyboard/Gamepad controls:
  * Z(A)QSD/Left Thumbstick: move
  * Space/Face Bottom Button: jump
  * Left CTRL/Right Trigger (hold): climb

To climb you have to maintain Left CTRL/Right Trigger the whole time.
Releasing this input or moving out of a grid while climbing will immediatly make the character fall.
It is possible to jump while climbing. If so, the character will have a slight cooldown before climbing again.

It is encouraged to test the climbing system in multiplayer with `Net PktLag=X`, `Net PktLoss=X`, `Net PktOrder=X` debug commands.

### Detecting when character can climb

While the map is created with a TileMap, it is not used to identify the tiles that character can climb.
Instead we use climbable volumes that are directly placed in the level to represent the climbable surfaces:

![Preview](https://github.com/Nauja/ue4-climb2d-sample/raw/master/docs/editor-climbable-volume.png)

The implementation of `ASampleClimbableVolume.cpp` is quite simple as it only serve to detect overlapping with
the character:

```cpp
void ASampleClimbableVolume::NotifyActorBeginOverlap(class AActor* Other)
{
    Super::NotifyActorBeginOverlap(Other);

    if (IsValid(Other) && !IsPendingKill())
    {
        StaticCast<ASampleCharacter*>(Other)->AddClimbableVolume(this);
    }
}

void ASampleClimbableVolume::NotifyActorEndOverlap(class AActor* Other)
{
    Super::NotifyActorEndOverlap(Other);

    if (IsValid(Other) && !IsPendingKill())
    {
        StaticCast<ASampleCharacter*>(Other)->RemoveClimbableVolume(this);
    }
}
```

In `ASampleCharacter.cpp` we enable climbing if the character is overlapping at least one climbing volume:

```cpp
void ASampleCharacter::AddClimbableVolume(ASampleClimbableVolume* Volume)
{
    Volumes.Add(Volume);
    SetClimbEnabled(true);
}

void ASampleCharacter::RemoveClimbableVolume(ASampleClimbableVolume* Volume)
{
    Volumes.Remove(Volume);
    if (Volumes.Num() == 0)
    {
        SetClimbEnabled(false);
    }
}
```

### Switching to climbing movement mode

The climbing system works in a similar way to the crouching system. When pressing/releasing the
`Climb` input, we toggle a boolean `bWantsToClimb` in the movement component:

```cpp
void ASampleCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    ...
    PlayerInputComponent->BindAction("Climb", IE_Pressed, this, &ASampleCharacter::StartClimb);
    PlayerInputComponent->BindAction("Climb", IE_Released, this, &ASampleCharacter::StopClimb);
    ...
}

void ASampleCharacter::StartClimb()
{
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(GetMovementComponent());

    if (MoveComponent)
    {
        if (CanClimb())
        {
            MoveComponent->bWantsToClimb = true;
        }
    }
}

void ASampleCharacter::StopClimb()
{
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(GetMovementComponent());

    if (MoveComponent)
    {
        MoveComponent->bWantsToClimb = false;
    }
}
```

This boolean is replicated to the server with a custom `FSavedMove` structure:

```cpp
void FSavedMove_SampleCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
    // Character -> Save
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(Character->GetMovementComponent());

    ...
    bWantsToClimb = MoveComponent->bWantsToClimb;

    Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
}

void FSavedMove_SampleCharacter::PrepMoveFor(ACharacter* Character)
{
    // Save -> Character
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(Character->GetCharacterMovement());
    if (MoveComponent)
    {
        ...
        MoveComponent->bWantsToClimb = bWantsToClimb;
    }

    Super::PrepMoveFor(Character);
}
```

And is used to switch from/to our custom climbing movement mode:

```cpp
void USampleCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
    ...

    // Proxies get replicated climb state.
    if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
    {
        // Check for a change in climb state. Players toggle climb by changing bWantsToClimb.
        const bool bIsClimbing = IsClimbing();
        if (bIsClimbing && (!bWantsToClimb || !CanClimbInCurrentState()))
        {
            UnClimb(false);
        }
        else if (!bIsClimbing && bWantsToClimb && CanClimbInCurrentState())
        {
            Climb(false);
        }
    }
}

void USampleCharacterMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

    // Proxies get replicated climb state.
    if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
    {
        // Unclimb if no longer allowed to be climbing
        if (IsClimbing() && !CanClimbInCurrentState())
        {
            UnClimb(false);
        }
    }
}
```

### Allowing to jump while climbing

Climbing is done by holding down the `Climb` input.

```cpp
bool USampleCharacterMovementComponent::CanAttemptJump() const
{
    if (CanEverJump() && IsClimbing())
    {
        return true;
    }

    return Super::CanAttemptJump();
}

bool USampleCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
    bool bWasClimbing = IsClimbing();

    if (Super::DoJump(bReplayingMoves))
    {
        if (bWasClimbing)
        {
            ClimbTimer = ClimbCooldown;
        }

        return true;
    }

    return false;
}
```

### Credits

Sprites are coming from [The Spriters Resource](https://www.spriters-resource.com/).

Font from [FontSpace](https://www.fontspace.com/atlantis-international-font-f31357).
