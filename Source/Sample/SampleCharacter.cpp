// Copyright Epic Games, Inc. All Rights Reserved.

#include "SampleCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "SampleCharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(SideScrollerCharacter, Log, All);

//////////////////////////////////////////////////////////////////////////
// ASampleCharacter

ASampleCharacter::ASampleCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USampleCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(28.0f);
	GetCapsuleComponent()->SetCapsuleRadius(16.0f);

	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 512.0f;
	SideViewCameraComponent->AspectRatio = 8.0f / 7.0f;
	SideViewCameraComponent->SetupAttachment(RootComponent);

	// Make the camera static and center on screen
	SideViewCameraComponent->SetAbsolute(true, true);
	SideViewCameraComponent->SetWorldLocationAndRotation(
		FVector(256.0f, 1000.0f, -224.0f),
		FQuat::MakeFromEuler(FVector(0.0f, 0.0f, -90.0f))
	);

	// Configure character movement
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 800.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 225.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
	// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
	// behavior on the edge of a ledge versus inclines by setting this to true or false
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

    // 	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
    // 	TextComponent->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.0f));
    // 	TextComponent->SetRelativeLocation(FVector(35.0f, 5.0f, 20.0f));
    // 	TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    // 	TextComponent->SetupAttachment(RootComponent);

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;
}

//////////////////////////////////////////////////////////////////////////
// Animation

void ASampleCharacter::UpdateAnimation()
{
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();

	UPaperFlipbook* DesiredAnimation = nullptr;
	USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(GetMovementComponent());
	if (MoveComponent && MoveComponent->IsClimbing())
	{
		DesiredAnimation = (PlayerSpeedSqr > 0.0f) ? ClimbingRunningAnimation : ClimbingIdleAnimation;
	}
	else
	{
		DesiredAnimation = (PlayerSpeedSqr > 0.0f) ? RunningAnimation : IdleAnimation;
	}

	if(DesiredAnimation && GetSprite()->GetFlipbook() != DesiredAnimation 	)
	{
		GetSprite()->SetFlipbook(DesiredAnimation);
	}
}

void ASampleCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateCharacter();	
}


//////////////////////////////////////////////////////////////////////////
// Input

void ASampleCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Climb", IE_Pressed, this, &ASampleCharacter::StartClimb);
	PlayerInputComponent->BindAction("Climb", IE_Released, this, &ASampleCharacter::StopClimb);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASampleCharacter::MoveRight);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &ASampleCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ASampleCharacter::TouchStopped);
}

void ASampleCharacter::MoveRight(float Value)
{
	/*UpdateChar();*/

	// Apply the input to the character motion
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

void ASampleCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Jump on any touch
	Jump();
}

void ASampleCharacter::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Cease jumping once touch stopped
	StopJumping();
}

void ASampleCharacter::UpdateCharacter()
{
	// Update animation to match the motion
	UpdateAnimation();

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();
	float TravelDirection = PlayerVelocity.X;
	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}

void ASampleCharacter::SetClimbEnabled(bool bIsEnabled)
{
	USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(GetMovementComponent());

	if (MoveComponent)
	{
		MoveComponent->bClimbEnabled = bIsEnabled;
	}
}

void ASampleCharacter::StartClimb()
{
	Climb();
}

void ASampleCharacter::StopClimb()
{
	UnClimb();
}

bool ASampleCharacter::CanClimb() const
{
	USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(GetMovementComponent());

	if (!MoveComponent || MoveComponent->IsClimbing() || !MoveComponent->CanEverClimb())
		return false;
	
	return GetRootComponent() && !GetRootComponent()->IsSimulatingPhysics();
}

void ASampleCharacter::Climb(bool bClientSimulation)
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

void ASampleCharacter::UnClimb(bool bClientSimulation)
{
	USampleCharacterMovementComponent* MoveComponent = Cast<USampleCharacterMovementComponent>(GetMovementComponent());

	if (MoveComponent)
	{
		MoveComponent->bWantsToClimb = false;
	}
}


void ASampleCharacter::OnEndClimb()
{
}

void ASampleCharacter::OnStartClimb()
{
}