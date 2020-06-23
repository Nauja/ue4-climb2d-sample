#include "SampleCharacterMovementComponent.h"
#include "SampleCharacter.h"
#include "GameFramework/Character.h"

USampleCharacterMovementComponent::USampleCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bClimbEnabled(false)
    , BrakingDecelerationClimbing(0.0f)
    , ClimbCooldown(0.0f)
    , ClimbTimer(0.0f)
    , bWantsToClimb(false)
{}

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

void USampleCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

    if (ClimbTimer > 0.0f)
    {
        ClimbTimer -= DeltaSeconds;
        if (ClimbTimer < 0.0f)
        {
            ClimbTimer = 0.0f;
        }
    }

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

bool USampleCharacterMovementComponent::CanClimbInCurrentState() const
{
    return bClimbEnabled && ClimbTimer <= 0.0f && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
}

bool USampleCharacterMovementComponent::IsClimbing() const
{
    return (MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ESampleMovementMode::MOVE_Climbing) && UpdatedComponent;
}

void USampleCharacterMovementComponent::Climb(bool bClientSimulation)
{
	if (!HasValidData())
	{
		return;
	}

	if (!bClientSimulation && !CanClimbInCurrentState())
	{
		return;
	}

    ASampleCharacter* Owner = StaticCast<ASampleCharacter*>(CharacterOwner);
    SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ESampleMovementMode::MOVE_Climbing);
    Velocity = FVector::ZeroVector;
    Owner->ResetJumpState();
}


void USampleCharacterMovementComponent::UnClimb(bool bClientSimulation)
{
	if (!HasValidData())
	{
		return;
	}

    ASampleCharacter* Owner = StaticCast<ASampleCharacter*>(CharacterOwner);
    SetMovementMode(EMovementMode::MOVE_Falling);
    ClimbTimer = ClimbCooldown;
}

void USampleCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
    switch ((ESampleMovementMode)CustomMovementMode)
    {
    case ESampleMovementMode::MOVE_Climbing:
        PhysCustomClimbing(deltaTime, Iterations);
        break;
    default:
        Super::PhysCustom(deltaTime, Iterations);
    }
}

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

FNetworkPredictionData_Client* USampleCharacterMovementComponent::GetPredictionData_Client() const
{
    // Should only be called on client in network games
    check(CharacterOwner != NULL);

    if (!ClientPredictionData)
    {
        USampleCharacterMovementComponent* MutableThis = const_cast<USampleCharacterMovementComponent*>(this);
        MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_SampleCharacter(*this);
    }

    return ClientPredictionData;
}

void USampleCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
    if (!CharacterOwner)
    {
        return;
    }

    Super::UpdateFromCompressedFlags(Flags);

    bWantsToClimb = ((Flags & FSavedMove_SampleCharacter::FLAG_ClimbPressed) != 0);
}

FSavedMove_SampleCharacter::FSavedMove_SampleCharacter()
    : ClimbTimer(0.0f)
    , bWantsToClimb(false)
    , ClimbTimerThresholdCombine(0.01f)
{}

FSavedMove_SampleCharacter::~FSavedMove_SampleCharacter()
{}

void FSavedMove_SampleCharacter::Clear()
{
    Super::Clear();

    ClimbTimer = 0.0f;
    bWantsToClimb = false;
}

void FSavedMove_SampleCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
    // Character -> Save
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(Character->GetMovementComponent());

    ClimbTimer = MoveComponent->ClimbTimer;
    bWantsToClimb = MoveComponent->bWantsToClimb;

    Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
}

void FSavedMove_SampleCharacter::PrepMoveFor(ACharacter* Character)
{
    // Save -> Character
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(Character->GetCharacterMovement());
    if (MoveComponent)
    {
        MoveComponent->ClimbTimer = ClimbTimer;
        MoveComponent->bWantsToClimb = bWantsToClimb;
    }

    Super::PrepMoveFor(Character);
}

uint8 FSavedMove_SampleCharacter::GetCompressedFlags() const
{
    uint8 Result = Super::GetCompressedFlags();

    if (bWantsToClimb)
    {
        Result |= FLAG_ClimbPressed;
    }

    return Result;
}

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

FNetworkPredictionData_Client_SampleCharacter::FNetworkPredictionData_Client_SampleCharacter(const UCharacterMovementComponent& ClientMovement)
    : FNetworkPredictionData_Client_Character(ClientMovement)
{
}

FNetworkPredictionData_Client_SampleCharacter::~FNetworkPredictionData_Client_SampleCharacter()
{
}

FSavedMovePtr FNetworkPredictionData_Client_SampleCharacter::AllocateNewMove()
{
    return FSavedMovePtr(new FSavedMove_SampleCharacter());
}
