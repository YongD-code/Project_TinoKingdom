// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTaskAttack.h"

#include "AIController.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

UBTTaskAttack::UBTTaskAttack()
{
	NodeName = TEXT("Attack");
}

EBTNodeResult::Type UBTTaskAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (EnemyCharacter == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	const bool bAttackStarted = EnemyCharacter->RequestAttack();

	return bAttackStarted ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}