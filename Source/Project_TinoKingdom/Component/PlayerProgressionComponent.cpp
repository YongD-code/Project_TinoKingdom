// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerProgressionComponent.h"

#include "Curves/CurveFloat.h"

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
