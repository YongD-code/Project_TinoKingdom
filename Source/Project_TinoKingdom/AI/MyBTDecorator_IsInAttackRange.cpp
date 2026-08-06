// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBTDecorator_IsInAttackRange.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

UMyBTDecorator_IsInAttackRange::UMyBTDecorator_IsInAttackRange()
{
	NodeName = TEXT("Is In Attack Range");
}

bool UMyBTDecorator_IsInAttackRange::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
) const
{
	const bool bParentResult = Super::CalculateRawConditionValue(OwnerComp, NodeMemory);
	if (!bParentResult)
	{
		return false;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return false;
	}

	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (EnemyCharacter == nullptr)
	{
		return false;
	}

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (BlackboardComponent == nullptr)
	{
		return false;
	}

	AActor* TargetActor = Cast<AActor>(
		BlackboardComponent->GetValueAsObject(AEnemyAIController::TargetPlayer)
	);

	if (TargetActor == nullptr)
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist(
		EnemyCharacter->GetActorLocation(),
		TargetActor->GetActorLocation()
	);

	return DistanceToTarget <= EnemyCharacter->GetAttackRange();
}

