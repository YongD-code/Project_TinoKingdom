// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTaskAttack.h"

#include "AIController.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

UBTTaskAttack::UBTTaskAttack()
{
	NodeName = TEXT("Attack");
}

EBTNodeResult::Type UBTTaskAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AEnemyCharacter* EnemyCharacter = AIController != nullptr
		? Cast<AEnemyCharacter>(AIController->GetPawn())
		: nullptr;

	if (EnemyCharacter == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	return EnemyCharacter->RequestAttack()
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}