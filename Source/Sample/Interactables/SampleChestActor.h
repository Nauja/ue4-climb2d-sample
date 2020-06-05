// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "SampleInteractableActor.h"
#include "SampleChestActor.generated.h"

UCLASS(config = Game, Blueprintable, BlueprintType)
class SAMPLE_API USampleChestConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	USampleChestConfig();

	/** Sprite displayed when the chest is closed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sample)
	class UPaperSprite* Closed;

	/** Sprite displayed when the chest is opened */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sample)
	class UPaperSprite* Opened;
};

UCLASS(config = Game, BlueprintType)
class SAMPLE_API ASampleChestActor : public ASampleInteractableActor
{
	GENERATED_BODY()

public:
	static FName ClosedSpriteComponentName;
	static FName OpenedSpriteComponentName;

	ASampleChestActor(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void BeginPlay() override;
	virtual void Interact_Implementation(class AActor* Other) override;

	FORCEINLINE class USampleChestConfig* GetChestConfig() const { return ChestConfig; }

protected:
	virtual void Reset() override;
	
private:
	UPROPERTY()
	bool bIsOpened;

	UPROPERTY(Category = Sample, EditAnywhere)
	class USampleChestConfig* ChestConfig;

	UPROPERTY(Category = Sample, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UPaperSpriteComponent* ClosedComponent;

	UPROPERTY(Category = Sample, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UPaperSpriteComponent* OpenedComponent;

	void RefreshConfig();
	void SetOpened(bool bState);
};
