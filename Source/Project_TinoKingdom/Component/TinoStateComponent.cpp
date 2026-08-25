// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoStateComponent.h"

#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"


void UTinoStateComponent::AddStateTag(const FGameplayTag& StateTag)
{
	if (!StateTag.IsValid())
	{
		return;
	}
	
	int32& Count = StateTagCounts.FindOrAdd(StateTag);
	++Count;
	
	if (Count == 1)
	{
		ActiveStateTags.AddTag(StateTag);
	}
}

void UTinoStateComponent::RemoveStateTag(const FGameplayTag& StateTag)
{
	int32* Count = StateTagCounts.Find(StateTag);

	// 추가된 적 없는 태그를 제거하려 하면 널 역참조가 되므로 조용히 넘어간다.
	if (Count == nullptr)
	{
		return;
	}

	--(*Count);

	if (*Count <= 0)
	{
		StateTagCounts.Remove(StateTag);
		ActiveStateTags.RemoveTag(StateTag);
	}
}

bool UTinoStateComponent::HasStateTag(const FGameplayTag& StateTag) const
{
	return ActiveStateTags.HasTag(StateTag);
}

bool UTinoStateComponent::HasAnyStateTags(const FGameplayTagContainer& StateTags) const
{
	return ActiveStateTags.HasAny(StateTags);
}

bool UTinoStateComponent::CanPerformAction(ETinoAction Action) const
{
	if (HasStateTag(TinoGameplayTags::State_Dead))
	{
		return false;
	}
	// 대화 중에는 모든 게임플레이 행동을 막는다.
	if (HasStateTag(TinoGameplayTags::State_InDialogue))
	{
		return false;
	}

	const bool bAttacking = HasStateTag(TinoGameplayTags::State_Action_Attacking);
	const bool bDodging = HasStateTag(TinoGameplayTags::State_Action_Dodging);
	const bool bReacting = HasStateTag(TinoGameplayTags::State_Action_HitReacting);

	switch (Action)
	{
	case ETinoAction::Attack:
		return !bDodging && !bReacting;
	case ETinoAction::Move:
	case ETinoAction::Jump:
	case ETinoAction::Sprint:
	case ETinoAction::Dodge:
		return !bDodging && !bReacting && !bAttacking;
	}
	
	return false;
}
