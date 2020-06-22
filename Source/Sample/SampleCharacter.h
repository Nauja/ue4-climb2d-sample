// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "SampleCharacter.generated.h"

class UTextRenderComponent;
class ASampleClimbableVolume;

/**
 * This class is the default character for Sample, and it is responsible for all
 * physical interaction between the player and the world.
 *
 * The capsule component (inherited from ACharacter) handles collision with the world
 * The CharacterMovementComponent (inherited from ACharacter) handles movement of the collision capsule
 * The Sprite component (inherited from APaperCharacter) handles the visuals
 */
UCLASS(config=Game)
class ASampleCharacter : public APaperCharacter
{
	GENERATED_BODY()

public:
	ASampleCharacter(const FObjectInitializer& ObjectInitializer);

	/** Returns SideViewCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }

	virtual void AddClimbableVolume(ASampleClimbableVolume* Volume);
	virtual void RemoveClimbableVolume(ASampleClimbableVolume* Volume);
	virtual void SetClimbEnabled(bool bIsEnabled);

	/** Locally when user start pressing the Climb button */
	UFUNCTION(BlueprintCallable, Category = Character)
	virtual void StartClimb();

	/** Locally when user stop pressing the Climb button */
	UFUNCTION(BlueprintCallable, Category = Character)
	virtual void StopClimb();

	/** Request the character to start climbing. The request is processed on the next update of the SampleCharacterMovementComponent. */
	UFUNCTION(BlueprintCallable, Category=Character, meta=(HidePin="bClientSimulation"))
	virtual void Climb(bool bClientSimulation = false);

	/** Request the character to stop climbing. The request is processed on the next update of the SampleCharacterMovementComponent. */
	UFUNCTION(BlueprintCallable, Category=Character, meta=(HidePin="bClientSimulation"))
	virtual void UnClimb(bool bClientSimulation = false);

	/** @return true if this character is currently able to climb (and is not currently climbing) */
	UFUNCTION(BlueprintCallable, Category=Character)
	virtual bool CanClimb() const;

	/** Called when Character stops climbing. Called on non-owned Characters through bIsClimbing replication. */
	virtual void OnEndClimb();

	/** Called when Character climbs. Called on non-owned Characters through bIsClimbing replication. */
	virtual void OnStartClimb();

protected:
	void UpdateAnimation();
	void MoveRight(float Value);
	void MoveUp(float Value);
	void UpdateCharacter();
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	
	// The animation to play while running around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animations)
	class UPaperFlipbook* RunningAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	class UPaperFlipbook* IdleAnimation;

	// The animation to play while climbing and running around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animations)
	class UPaperFlipbook* ClimbingRunningAnimation;

	// The animation to play while climbing and idle (standing still)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	class UPaperFlipbook* ClimbingIdleAnimation;

private:
	/** Side view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera, meta=(AllowPrivateAccess="true"))
	class UCameraComponent* SideViewCameraComponent;

	UTextRenderComponent* TextComponent;
	virtual void Tick(float DeltaSeconds) override;

	/** Store overlapping volumes */
	UPROPERTY(transient)
	TSet<ASampleClimbableVolume*> Volumes;
};
