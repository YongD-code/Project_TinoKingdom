// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackHitWindowAnimNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Project_TinoKingdom/Interface/CombatAttackAnimationInterface.h"

void UAttackHitWindowAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                  float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (!IsValid(MeshComp))
	{
		return;
	}
	
	ICombatAttackAnimationInterface* CombatInterface = Cast<ICombatAttackAnimationInterface>(MeshComp->GetOwner());
	if (CombatInterface != nullptr)
	{
		CombatInterface->BeginAttackHitWindow();
	}
}

void UAttackHitWindowAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (!IsValid(MeshComp))
	{
		return;
	}
	
	ICombatAttackAnimationInterface* CombatInterface = Cast<ICombatAttackAnimationInterface>(MeshComp->GetOwner());
	if (CombatInterface != nullptr)
	{
		CombatInterface->TickAttackHitWindow(FrameDeltaTime);
	}
}

void UAttackHitWindowAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	ICombatAttackAnimationInterface* CombatInterface = Cast<ICombatAttackAnimationInterface>(MeshComp->GetOwner());

	if (CombatInterface != nullptr)
	{
		CombatInterface->EndAttackHitWindow();
	}
}
