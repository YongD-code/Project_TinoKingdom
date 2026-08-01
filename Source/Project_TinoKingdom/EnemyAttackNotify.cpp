// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAttackNotify.h"
#include "Components/SkeletalMeshComponent.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

void UEnemyAttackNotify::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference
)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(MeshComp->GetOwner());
	if (EnemyCharacter == nullptr)
	{
		return;
	}

	EnemyCharacter->PerformAttackTrace();
}