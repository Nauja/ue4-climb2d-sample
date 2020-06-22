// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SampleClimbableVolume.generated.h"

UCLASS(config=Game)
class ASampleClimbableVolume : public AActor
{
	GENERATED_BODY()

public:
	ASampleClimbableVolume(const FObjectInitializer& ObjectInitializer);

	virtual void NotifyActorBeginOverlap(class AActor* Other) override;
	virtual void NotifyActorEndOverlap(class AActor* Other) override;

	FORCEINLINE class UBoxComponent* GetBoxComponent() const { return BoxComponent; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UBoxComponent* BoxComponent;
};
