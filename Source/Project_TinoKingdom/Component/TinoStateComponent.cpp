// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoStateComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"

void UTinoStateComponent::BeginPlay()
{
	Super::BeginPlay();

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	if (!ensureMsgf(
		AbilitySystemInterface != nullptr,
		TEXT("TinoStateComponent의 소유자가 IAbilitySystemInterface를 구현하지 않았습니다.")
	))
	{
		return;
	}

	AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
	ensureMsgf(
		AbilitySystemComponent != nullptr,
		TEXT("TinoStateComponent가 AbilitySystemComponent를 찾지 못했습니다.")
	);
}

void UTinoStateComponent::AddStateTag(const FGameplayTag& StateTag)
{
	if (AbilitySystemComponent == nullptr || !StateTag.IsValid())
	{
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTag(StateTag);
}

void UTinoStateComponent::RemoveStateTag(const FGameplayTag& StateTag)
{
	if (
		AbilitySystemComponent == nullptr
		|| !StateTag.IsValid()
		|| AbilitySystemComponent->GetTagCount(StateTag) <= 0
	)
	{
		return;
	}

	AbilitySystemComponent->RemoveLooseGameplayTag(StateTag);
}

bool UTinoStateComponent::HasStateTag(const FGameplayTag& StateTag) const
{
	return AbilitySystemComponent != nullptr
		&& StateTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(StateTag);
}

bool UTinoStateComponent::HasAnyStateTags(const FGameplayTagContainer& StateTags) const
{
	return AbilitySystemComponent != nullptr
		&& AbilitySystemComponent->HasAnyMatchingGameplayTags(StateTags);
}

bool UTinoStateComponent::CanPerformAction(ETinoAction Action) const
{
	if (AbilitySystemComponent == nullptr)
	{
		return false;
	}

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
