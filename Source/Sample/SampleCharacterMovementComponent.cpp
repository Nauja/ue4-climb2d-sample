#include "SampleCharacterMovementComponent.h"
#include "SampleCharacter.h"
#include "GameFramework/Character.h"

USampleCharacterMovementComponent::USampleCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bClimbEnabled(false)
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

bool USampleCharacterMovementComponent::CanAttemptJump() const
{
    return IsJumpAllowed() &&
        !bWantsToCrouch &&
        (IsMovingOnGround() || IsFalling() || IsClimbing()); // Falling included for double-jump and non-zero jump hold time, but validated by character.
}

bool USampleCharacterMovementComponent::CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump)
{
/*    if (IsClimbing())
    {
        return false;
    }*/

    return Super::CheckFall(OldFloor, Hit, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump);
}

FVector USampleCharacterMovementComponent::NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const
{
/*    if (IsClimbing())
    {
        return FVector::ZeroVector;
    }*/
    return Super::NewFallVelocity(InitialVelocity, Gravity, DeltaTime);
}

void USampleCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

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

bool USampleCharacterMovementComponent::CanClimbInCurrentState() const
{
    return bClimbEnabled && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
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
    Owner->OnStartClimb();
}


void USampleCharacterMovementComponent::UnClimb(bool bClientSimulation)
{
	if (!HasValidData())
	{
		return;
	}

    ASampleCharacter* Owner = StaticCast<ASampleCharacter*>(CharacterOwner);
    if (!bClientSimulation)
    {
        SetMovementMode(EMovementMode::MOVE_Falling);
    }
    Owner->OnEndClimb();
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

	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		RestorePreAdditiveRootMotionVelocity();

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;
		Acceleration.Z = 0.f;

		// Apply acceleration
		if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{
			CalcVelocity(timeTick, GroundFriction, false, GetMaxBrakingDeceleration());
		}

		ApplyRootMotionToVelocity(timeTick);

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}

	if (IsMovingOnGround())
	{
		MaintainHorizontalGroundVelocity();
	}
}

FNetworkPredictionData_Client* USampleCharacterMovementComponent::GetPredictionData_Client() const
{
    // Should only be called on client in network games
    check(CharacterOwner != NULL);
    check(CharacterOwner->GetLocalRole() < ROLE_Authority);

    if (!ClientPredictionData)
    {
        USampleCharacterMovementComponent* MutableThis = const_cast<USampleCharacterMovementComponent*>(this);
        MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_SampleCharacter(*this);
    }

    return ClientPredictionData;
}

void USampleCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);

    bWantsToClimb = ((Flags & FSavedMove_SampleCharacter::FLAG_ClimbPressed) != 0);
}

FSavedMove_SampleCharacter::FSavedMove_SampleCharacter()
    : bWantsToClimb(false)
{}

FSavedMove_SampleCharacter::~FSavedMove_SampleCharacter()
{}

void FSavedMove_SampleCharacter::Clear()
{
    Super::Clear();

    bWantsToClimb = false;
}

void FSavedMove_SampleCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
    Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

    // Character -> Save
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(Character->GetMovementComponent());

    bWantsToClimb = MoveComponent->bWantsToClimb;
}

void FSavedMove_SampleCharacter::PrepMoveFor(ACharacter* Character)
{
    Super::PrepMoveFor(Character);

    // Save -> Character
    USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(Character->GetCharacterMovement());
    if (MoveComponent)
    {
        MoveComponent->bWantsToClimb = bWantsToClimb;
    }
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

    if (bWantsToClimb != SampleNewMove->bWantsToClimb)
    {
        return false;
    }

    return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

bool FSavedMove_SampleCharacter::IsImportantMove(const FSavedMovePtr& LastAckedMove) const
{
    const FSavedMove_SampleCharacter* SampleLastAckedMove = (FSavedMove_SampleCharacter*)&LastAckedMove;

    if (bWantsToClimb != SampleLastAckedMove->bWantsToClimb)
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
