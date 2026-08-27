// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerProgressionComponent.generated.h"

class UCurveFloat;
class UGameplayEffect;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerLevelChanged, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerExperienceChanged, int32, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerStatPointsChanged, int32);

enum class EPlayerStatType : uint8
{
	MaxHealth,
	MaxStamina,
	AttackPower,
	Defense
};

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
	
	bool TryUpgradeStat(EPlayerStatType StatType);
	
public:
	FOnPlayerLevelChanged OnLevelChanged;
	FOnPlayerExperienceChanged OnExperienceChanged;
	FOnPlayerStatPointsChanged OnStatPointsChanged;
	
	static constexpr int32 MaxLevel = 100;
	
private:
	bool TrySpendStatPoint();
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Experience")
	TObjectPtr<UCurveFloat> RequiredExperienceCurve;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	int32 CurrentLevel = 1;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	int32 CurrentExperience = 0;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	int32 UnspentStatPoints = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Stat Upgrade")
	TSubclassOf<UGameplayEffect> StatUpgradeEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Stat Upgrade")
	float MaxHealthIncreasePerPoint = 10.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Stat Upgrade")
	float MaxStaminaIncreasePerPoint = 10.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Stat Upgrade")
	float AttackPowerIncreasePerPoint = 2.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Stat Upgrade")
	float DefenseIncreasePerPoint = 2.f;
};
