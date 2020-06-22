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

    virtual float GetMaxSpeed() const override;
    virtual bool CanAttemptJump() const override;
    virtual bool CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump) override;
    virtual FVector NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const override;
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual bool CanClimbInCurrentState() const;
    FORCEINLINE virtual bool CanEverClimb() const { return true; };
    virtual bool IsClimbing() const;
    virtual void Climb(bool bClientSimulation);
    virtual void UnClimb(bool bClientSimulation);
    virtual void PhysCustom(float deltaTime, int32 Iterations) override;
    virtual void PhysCustomClimbing(float deltaTime, int32 Iterations);
    virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;

    /** If true, try to climb (or keep climbing) on next update. If false, try to stop climbing on next update. */
    UPROPERTY(Category = "Sample", VisibleInstanceOnly, BlueprintReadOnly)
    bool bClimbEnabled;
    
	/** The maximum climbing speed. */
	UPROPERTY(Category="Character Movement: Climbing", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxClimbSpeed;

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

    uint32 bWantsToClimb : 1;

    virtual void Clear() override;
    virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
    virtual void PrepMoveFor(ACharacter* Character) override;
    virtual uint8 GetCompressedFlags() const override;
    virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
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