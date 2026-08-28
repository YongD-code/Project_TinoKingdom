// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerProgressionComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Curves/CurveFloat.h"
#include "GameplayEffect.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"


// Sets default values for this component's properties
UPlayerProgressionComponent::UPlayerProgressionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

int32 UPlayerProgressionComponent::GetRequiredExperienceForNextLevel() const
{
	if (IsMaxLevel())
	{
		return 0;
	}
	if (!ensureMsgf(RequiredExperienceCurve != nullptr, TEXT("RequiredExperienceCurve가 지정되지 않았습니다.")))
	{
		return 0;
	}
	const float CurveValue = RequiredExperienceCurve->GetFloatValue(static_cast<float>(CurrentLevel));
	
	return FMath::RoundToInt(CurveValue);
}

void UPlayerProgressionComponent::AddExperience(int32 Amount)
{
	if (Amount <= 0 || IsMaxLevel())
	{
		return;
	}
	
	const int64 AccumulatedExperience = static_cast<int64>(CurrentExperience) + Amount;
	CurrentExperience = static_cast<int32>(FMath::Min(AccumulatedExperience, static_cast<int64>(MAX_int32)));
	
	bool bLevelUp = false;
	while (!IsMaxLevel())
	{
		const int32 RequiredExperience = GetRequiredExperienceForNextLevel();
		if (RequiredExperience <= 0)
		{
			break;
		}
		if (CurrentExperience < RequiredExperience)
		{
			break;
		}
		
		CurrentExperience -= RequiredExperience;
		++CurrentLevel;
		++UnspentStatPoints;

		bLevelUp = true;
		OnLevelChanged.Broadcast(CurrentLevel);
	}
	
	if (IsMaxLevel())
	{
		CurrentExperience = 0;
	}
	
	if (bLevelUp)
	{
		OnStatPointsChanged.Broadcast(UnspentStatPoints);
	}
	
	OnExperienceChanged.Broadcast(CurrentExperience, GetRequiredExperienceForNextLevel());
}

bool UPlayerProgressionComponent::TryUpgradeStat(EPlayerStatType StatType)
{
	if (UnspentStatPoints <= 0)
	{
		return false;
	}
	if (!ensureMsgf(StatUpgradeEffect != nullptr, TEXT("StatUpgradeEffect가 지정되지 않았습니다.")))
	{
		return false;
	}
	
	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
	
	float UpgradeAmount = 0.f;
	switch (StatType)
	{
	case EPlayerStatType::MaxHealth:
		UpgradeAmount = MaxHealthIncreasePerPoint;
		break;
	case EPlayerStatType::MaxStamina:
		UpgradeAmount = MaxStaminaIncreasePerPoint;
		break;
	case EPlayerStatType::AttackPower:
		UpgradeAmount = AttackPowerIncreasePerPoint;
		break;
	case EPlayerStatType::Defense:
		UpgradeAmount = DefenseIncreasePerPoint;
		break;
	default:
		return false;
	}
	
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());
	
	FGameplayEffectSpecHandle EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		StatUpgradeEffect, 1.f, EffectContext);
	if (!ensureMsgf(EffectSpecHandle.IsValid(), TEXT("스탯 강화 GameplayEffect Spec 생성 실패")))
	{
		return false;
	}
	
	FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
	EffectSpec->SetSetByCallerMagnitude(TinoGameplayTags::Data_StatUpgrade_MaxHealth,
		StatType == EPlayerStatType::MaxHealth ? UpgradeAmount : 0.f);
	EffectSpec->SetSetByCallerMagnitude(TinoGameplayTags::Data_StatUpgrade_MaxStamina,
		StatType == EPlayerStatType::MaxStamina ? UpgradeAmount : 0.f);
	EffectSpec->SetSetByCallerMagnitude(TinoGameplayTags::Data_StatUpgrade_AttackPower,
		StatType == EPlayerStatType::AttackPower ? UpgradeAmount : 0.f);
	EffectSpec->SetSetByCallerMagnitude(TinoGameplayTags::Data_StatUpgrade_Defense,
		StatType == EPlayerStatType::Defense ? UpgradeAmount : 0.f);
	
	const FActiveGameplayEffectHandle AppliedEffectHandle = 
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec);
	if (!AppliedEffectHandle.WasSuccessfullyApplied())
	{
		return false;
	}

	return TrySpendStatPoint();
}

bool UPlayerProgressionComponent::TrySpendStatPoint()
{
	if (UnspentStatPoints <= 0)
	{
		return false;
	}
	
	--UnspentStatPoints;
	OnStatPointsChanged.Broadcast(UnspentStatPoints);
	
	return true;
}
