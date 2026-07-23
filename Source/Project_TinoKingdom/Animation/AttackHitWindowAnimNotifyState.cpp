// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackHitWindowAnimNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Project_TinoKingdom/Component/TinoCombatComponent.h"

namespace 
{
	UTinoCombatComponent* FindCombatComponent(const USkeletalMeshComponent* MeshComp)
	{
		if (!IsValid(MeshComp))
		{
			return nullptr;
		}
		AActor* Owner = MeshComp->GetOwner();
		if (!IsValid(Owner))
		{
			return nullptr;
		}
		return Owner->FindComponentByClass<UTinoCombatComponent>();
	}
}

void UAttackHitWindowAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                  float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (UTinoCombatComponent* CombatComponent = FindCombatComponent(MeshComp))
	{
		CombatComponent->BeginAttackHitWindow();
	}
}

void UAttackHitWindowAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (UTinoCombatComponent* CombatComponent = FindCombatComponent(MeshComp))
	{
		CombatComponent->TickAttackHitWindow();
	}
}

void UAttackHitWindowAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (UTinoCombatComponent* CombatComponent = FindCombatComponent(MeshComp))
	{
		CombatComponent->EndAttackHitWindow();
	}
}
