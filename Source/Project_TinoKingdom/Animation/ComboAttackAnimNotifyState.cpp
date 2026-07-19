// Fill out your copyright notice in the Description page of Project Settings.


#include "ComboAttackAnimNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Project_TinoKingdom/Interface/CombatAttackAnimationInterface.h"

void UComboAttackAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                              float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (!IsValid(MeshComp))
	{
		return;
	}
	
	ICombatAttackAnimationInterface* CombatAnimationInterface =
		Cast<ICombatAttackAnimationInterface>(MeshComp->GetOwner());
	if (CombatAnimationInterface != nullptr)
	{
		CombatAnimationInterface->SetComboInputWindowOpen(true);
	}
}

void UComboAttackAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (!IsValid(MeshComp))
	{
		return;
	}

	ICombatAttackAnimationInterface* CombatAnimationInterface =
		Cast<ICombatAttackAnimationInterface>(MeshComp->GetOwner());
	if (CombatAnimationInterface != nullptr)
	{
		CombatAnimationInterface->SetComboInputWindowOpen(false);
	}
}
