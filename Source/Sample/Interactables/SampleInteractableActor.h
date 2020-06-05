// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "SampleInteractableActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBeginInteractableDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndInteractableDelegate);

UCLASS(config = Game, Blueprintable, BlueprintType)
class SAMPLE_API USampleInteractableConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	USampleInteractableConfig();

	/** Seconds before resetting the state of this interactable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sample)
	float ResetTimer;
};

UCLASS(config = Game, BlueprintType)
class SAMPLE_API ASampleInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	ASampleInteractableActor(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	void NotifyActorBeginOverlap(class AActor* Other) override;
	void NotifyActorEndOverlap(class AActor* Other) override;

	/** Called when at least one actor can interact with this interactable **/
	UPROPERTY(BlueprintAssignable, Category="Sample")
	FOnBeginInteractableDelegate OnBeginInteractable;

	/** Called when no more actors can interact with this interactable **/
	UPROPERTY(BlueprintAssignable, Category="Sample")
	FOnEndInteractableDelegate OnEndInteractable;
	
	/** [Server] Called from ASampleCharacter when pressing Interact button */
	UFUNCTION(BlueprintNativeEvent, Category = "Sample")
	void Interact(class AActor* Other);

	/** [Server] Set if the interaction is enabled or not */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Sample")
	void SetIsEnabled(bool State);

	FORCEINLINE class USampleInteractableConfig* GetConfig() const { return Config; }

protected:
	/** Called when at least one actor can interact with this interactable **/
	UFUNCTION(BlueprintNativeEvent, Category = "Sample")
	void NotifyBeginInteractable();

	/** Called when no more actors can interact with this interactable **/
	UFUNCTION(BlueprintNativeEvent, Category = "Sample")
	void NotifyEndInteractable();

	/** [Client] Called when bIsEnabled get replicated */
	UFUNCTION()
	virtual void OnRep_IsEnabled();

	/** [Server] Reset the interaction making it available again */
	virtual void Reset();

private:
	UPROPERTY(Category = Sample, EditAnywhere)
	class USampleInteractableConfig* Config;

	UPROPERTY(Category = Sample, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* SceneComponent;

	/** Indicate if the interaction is enabled **/
	UPROPERTY(replicated, ReplicatedUsing = OnRep_IsEnabled)
	bool bIsEnabled;

	UPROPERTY(transient)
	TSet<class AActor*> OverlappingActors;

	/** Timer before resetting the state of this interactable */
	FTimerHandle TimerHandle_Reset;
};
