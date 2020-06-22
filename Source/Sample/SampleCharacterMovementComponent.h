#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "SampleCharacterMovementComponent.generated.h"

enum class ESampleMovementMode : uint8
{
    MOVE_None = 0,
    MOVE_Climbing
};

UCLASS(ClassGroup = "Sample")
class SAMPLE_API USampleCharacterMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    USampleCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

    /** Specify max speed when climbing */
    virtual float GetMaxSpeed() const override;
    /** Specify max braking deceleration when climbing */
    virtual float GetMaxBrakingDeceleration() const;
    /** Allow to jump when climbing */
    virtual bool CanAttemptJump() const override;
    virtual bool DoJump(bool bReplayingMoves) override;
    /** Apply bWantsToClimb before movement */
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
    /** Allow to climb or not */
    virtual bool CanClimbInCurrentState() const;
    /** If we are in the MOVE_Climbing movement mode */
    virtual bool IsClimbing() const;
    /** Change movement mode to MOVE_Climbing */
    virtual void Climb(bool bClientSimulation);
    /** Change movement mode to MOVE_Falling */
    virtual void UnClimb(bool bClientSimulation);
    /** Custom physics for MOVE_Climbing movement mode */
    virtual void PhysCustom(float deltaTime, int32 Iterations) override;
    virtual void PhysCustomClimbing(float deltaTime, int32 Iterations);
    /** Custom prediction data sent to client */
    virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;

    /** If true, try to climb (or keep climbing) on next update. If false, try to stop climbing on next update. */
    UPROPERTY(Category = "Sample", VisibleInstanceOnly, BlueprintReadOnly)
    bool bClimbEnabled;
    
	/** The maximum climbing speed. */
	UPROPERTY(Category="Character Movement: Climbing", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxClimbSpeed;

	/**
	 * Deceleration when climbing and not applying acceleration.
	 * @see MaxAcceleration
	 */
	UPROPERTY(Category="Character Movement: Climbing", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationClimbing;

    /**
     * Cooldown before the character can climb again after leaving the climbing state by jumping.
     */
    UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
    float ClimbCooldown;

    /** Remaining time before the character can climb again. */
    UPROPERTY(Category = "Sample", VisibleInstanceOnly, BlueprintReadOnly)
    float ClimbTimer;

    /** If true, try to climb (or keep climbing) on next update. If false, try to stop climbing on next update. */
    UPROPERTY(Category = "Sample", VisibleInstanceOnly, BlueprintReadOnly)
    bool bWantsToClimb;
};

// Custom FSavedMove_Character used to save custom inputs.
class SAMPLE_API FSavedMove_SampleCharacter : public FSavedMove_Character
{
    typedef FSavedMove_Character Super;

public:
    FSavedMove_SampleCharacter();
    virtual ~FSavedMove_SampleCharacter();

    float ClimbTimer;
    uint32 bWantsToClimb : 1;
    float ClimbTimerThresholdCombine;

    virtual void Clear() override;
    virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
    virtual void PrepMoveFor(ACharacter* Character) override;
    virtual uint8 GetCompressedFlags() const override;
    virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
    virtual void CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation) override;
    bool IsImportantMove(const FSavedMovePtr& LastAckedMove) const;

    enum CustomCompressedFlags : uint8
    {
        FLAG_ClimbPressed = 0x10
    };
};

// Custom FNetworkPredictionData_Client_Character used to replace FSavedMove_Character by FSavedMove_SampleCharacter.
class SAMPLE_API FNetworkPredictionData_Client_SampleCharacter : public FNetworkPredictionData_Client_Character
{
public:
    FNetworkPredictionData_Client_SampleCharacter(const UCharacterMovementComponent& ClientMovement);
    virtual ~FNetworkPredictionData_Client_SampleCharacter();

    virtual FSavedMovePtr AllocateNewMove() override;
};