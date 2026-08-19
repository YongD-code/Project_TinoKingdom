// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "TinoStateComponent.generated.h"


UENUM(BlueprintType)
enum class ETinoAction : uint8
{
	Move,
	Attack,
	Jump,
	Sprint,
	Dodge
};

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UTinoStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	void AddStateTag(const FGameplayTag& StateTag);
	void RemoveStateTag(const FGameplayTag& StateTag);
	
	bool HasStateTag(const FGameplayTag& StateTag) const;
	bool HasAnyStateTags(const FGameplayTagContainer& StateTags) const;
	
	bool CanPerformAction(ETinoAction Action) const;

public:	
	UPROPERTY(VisibleAnywhere, Category = "State")
	FGameplayTagContainer ActiveStateTags;
	
	TMap<FGameplayTag, int32> StateTagCounts;
};
