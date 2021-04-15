# ue4-climb2d-sample

![UE4](https://img.shields.io/badge/UE4-4.25+-blue)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/Nauja/ue4-climb2d-sample/master/LICENSE)

Sample of a custom climbing movement done with Paper2D.

![Preview](https://github.com/Nauja/ue4-climb2d-sample/raw/media/preview.gif)

This project is an example of how to write a custom climbing movement in a Paper2D game, with the constraint of
being fully replicated over network.

Prerequisites:
  * [Pixel Perfect 2D Sample](https://github.com/Nauja/ue4-pixelperfect2d-sample)

## Table of contents:

- [Keyboard/Gamepad controls](#keyboardgamepad_controls)
- [Detecting when character can climb](#detecting-when-character-can-climb)
- [Switching to climbing movement mode](#switching-to-climbing-movement-mode)
- [Moving while climbing](#moving-while-climbing)
- [Allowing to jump while climbing](#allowing-to-jump-while-climbing)

## Keyboard/Gamepad controls

  * Z(A)QSD/Left Thumbstick: move
  * Space/Face Bottom Button: jump
  * Left CTRL/Right Trigger (hold): climb

To climb you have to maintain Left CTRL/Right Trigger the whole time.
Releasing this input or moving out of a grid while climbing will immediatly make the character fall.
It is possible to jump while climbing. If so, the character will have a slight cooldown before climbing again.

It is encouraged to test the climbing system in multiplayer with `Net PktLag=X`, `Net PktLoss=X`, `Net PktOrder=X` debug commands.

This is the result in multiplayer with `Net PktLoss=10` (client on left):

![PktLoss](https://github.com/Nauja/ue4-climb2d-sample/raw/media/pktloss.gif)

This is the result in multiplayer with `Net PktLag=100` (client on left):

![PktLag](https://github.com/Nauja/ue4-climb2d-sample/raw/media/pktlag.gif)

## Detecting when character can climb

While the map is created with a TileMap, it is not used to identify the tiles that character can climb.
Instead we use climbable volumes that are directly placed in the level to represent the climbable surfaces:

![Preview](https://github.com/Nauja/ue4-climb2d-sample/raw/media/editor-climbable-volume.png)

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

## Switching to climbing movement mode

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

## Moving while climbing

Entering the climbing state sets a custom movement mode:

```cpp
SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ESampleMovementMode::MOVE_Climbing);
```

In this mode, all the physics is handled in `PhysCustomClimbing` that is a copy of `PhysFlying` with slight modifications:

```cpp
void USampleCharacterMovementComponent::PhysCustomClimbing(float deltaTime, int32 Iterations)
{
    if (deltaTime < MIN_TICK_TIME)
    {
        return;
    }

    RestorePreAdditiveRootMotionVelocity();

    // Apply acceleration
    if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
    {
        CalcVelocity(deltaTime, GroundFriction, false, GetMaxBrakingDeceleration());
    }

    ApplyRootMotionToVelocity(deltaTime);

    Iterations++;
    bJustTeleported = false;

    FVector OldLocation = UpdatedComponent->GetComponentLocation();
    const FVector Adjusted = Velocity * deltaTime;
    FHitResult Hit(1.f);
    SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

    if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
    {
        Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
    }
}
```

The important part is to override `GetMaxSpeed` and `GetMaxBrakingDeceleration` functions:

```cpp
float USampleCharacterMovementComponent::GetMaxSpeed() const
{
    if (IsClimbing())
    {
        return MaxClimbSpeed;
    }

    return Super::GetMaxSpeed();
}

float USampleCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
    if (IsClimbing())
    {
        return BrakingDecelerationClimbing;
    }

    return Super::GetMaxBrakingDeceleration();
}
```

Inputs are handled in `ASampleCharacter.cpp`:

```cpp

void ASampleCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    ...
    PlayerInputComponent->BindAxis("MoveRight", this, &ASampleCharacter::MoveRight);
    PlayerInputComponent->BindAxis("MoveUp", this, &ASampleCharacter::MoveUp);
}

void ASampleCharacter::MoveRight(float Value)
{
    // Apply the input to the character motion
    AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

void ASampleCharacter::MoveUp(float Value)
{
    // Can only move up if climbing
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(GetMovementComponent());

    if (MoveComponent && MoveComponent->IsClimbing())
    {
        AddMovementInput(FVector(0.0f, 0.0f, 1.0f), Value);
    }
}
```

## Allowing to jump while climbing

Climbing is done by holding down the `Climb` input, and it is possible to jump while climbing.
This is done by overriding `CanAttemptJump` and adding a climbing cooldown in `DoJump` to prevent the character from re-entering the climbing state right after:

```cpp
bool USampleCharacterMovementComponent::CanClimbInCurrentState() const
{
    return bClimbEnabled && ClimbTimer <= 0.0f && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
}

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

As for `bWantsToClimb`, this cooldown is replicated via the custom `FSavedMove`:

```cpp
void FSavedMove_SampleCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
    // Character -> Save
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(Character->GetMovementComponent());

    ClimbTimer = MoveComponent->ClimbTimer;
    ...

    Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
}

void FSavedMove_SampleCharacter::PrepMoveFor(ACharacter* Character)
{
    // Save -> Character
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(Character->GetCharacterMovement());
    if (MoveComponent)
    {
        MoveComponent->ClimbTimer = ClimbTimer;
        ...
    }

    Super::PrepMoveFor(Character);
}
```

It is important to implement `IsImportantMove`, `CanCombineWith` and `CombineWith` functions correctly so we don't
send too many packets between the client and server:

```cpp
bool FSavedMove_SampleCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
    const FSavedMove_SampleCharacter* SampleNewMove = (FSavedMove_SampleCharacter*)&NewMove;

    if (!FMath::IsNearlyEqual(ClimbTimer, SampleNewMove->ClimbTimer, ClimbTimerThresholdCombine))
    {
        return false;
    }

    if ((ClimbTimer <= 0.0f) != (SampleNewMove->ClimbTimer <= 0.0f))
    {
        return false;
    }

    if ((ClimbTimer > 0.0f) != (SampleNewMove->ClimbTimer > 0.0f))
    {
        return false;
    }

    return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_SampleCharacter::CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation)
{
    const FSavedMove_SampleCharacter* SampleNewMove = (FSavedMove_SampleCharacter*)&OldMove;

    ClimbTimer = SampleNewMove->ClimbTimer;

    Super::CombineWith(OldMove, InCharacter, PC, OldStartLocation);
}

bool FSavedMove_SampleCharacter::IsImportantMove(const FSavedMovePtr& LastAckedMove) const
{
    const FSavedMove_SampleCharacter* SampleLastAckedMove = (FSavedMove_SampleCharacter*)&LastAckedMove;

    if (!FMath::IsNearlyEqual(ClimbTimer, SampleLastAckedMove->ClimbTimer, ClimbTimerThresholdCombine))
    {
        return true;
    }

    return Super::IsImportantMove(LastAckedMove);
}
```

## Credits

Sprites are coming from [The Spriters Resource](https://www.spriters-resource.com/).

Font from [FontSpace](https://www.fontspace.com/atlantis-international-font-f31357).

## License

Licensed under the [MIT](LICENSE) License.
