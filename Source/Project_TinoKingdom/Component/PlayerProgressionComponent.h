// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerProgressionComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerLevelChanged, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerExperienceChanged, int32, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerStatPointsChanged, int32);

class UCurveFloat;

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UPlayerProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPlayerProgressionComponent();
	
	int32 GetCurrentLevel() const { return CurrentLevel; }
	int32 GetCurrentExperience() const { return CurrentExperience; }
	int32 GetUnspentStatPoints() const { return UnspentStatPoints; }
	
	int32 GetRequiredExperienceForNextLevel() const;
	
	bool IsMaxLevel() const { return CurrentLevel >= MaxLevel; }
	
	void AddExperience(int32 Amount);
	
	bool TrySpendStatPoint();
	
public:
	FOnPlayerLevelChanged OnLevelChanged;
	FOnPlayerExperienceChanged OnExperienceChanged;
	FOnPlayerStatPointsChanged OnStatPointsChanged;
	
	static constexpr int32 MaxLevel = 100;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Experience")
	TObjectPtr<UCurveFloat> RequiredExperienceCurve;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	int32 CurrentLevel = 1;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	int32 CurrentExperience = 0;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	int32 UnspentStatPoints = 0;
};
