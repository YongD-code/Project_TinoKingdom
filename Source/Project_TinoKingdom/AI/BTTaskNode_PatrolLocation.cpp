// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTaskNode_PatrolLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/TargetPoint.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

struct FPatrolTaskMemory
{
	int32 CurrentIndex = 0;
};

UBTTaskNode_PatrolLocation::UBTTaskNode_PatrolLocation()
{
	NodeName = TEXT("Set Patrol Location");

	PatrolLocationKey.AddVectorFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTaskNode_PatrolLocation, PatrolLocationKey)
	);
}

uint16 UBTTaskNode_PatrolLocation::GetInstanceMemorySize() const
{
	return sizeof(FPatrolTaskMemory);
}

EBTNodeResult::Type UBTTaskNode_PatrolLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AEnemyCharacter* EnemyCharacter = AIController != nullptr
		? Cast<AEnemyCharacter>(AIController->GetPawn())
		: nullptr;

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (EnemyCharacter == nullptr || BlackboardComponent == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	const TArray<TObjectPtr<ATargetPoint>>& PatrolPoints = EnemyCharacter->GetPatrolPoints();

	if (PatrolPoints.Num() == 0)
	{
		return EBTNodeResult::Failed;
	}

	FPatrolTaskMemory* Memory = reinterpret_cast<FPatrolTaskMemory*>(NodeMemory);

	for (int32 TryCount = 0; TryCount < PatrolPoints.Num(); ++TryCount)
	{
		const int32 PointIndex = Memory->CurrentIndex % PatrolPoints.Num();
		Memory->CurrentIndex = (Memory->CurrentIndex + 1) % PatrolPoints.Num();

		ATargetPoint* PatrolPoint = PatrolPoints[PointIndex].Get();
		if (PatrolPoint == nullptr)
		{
			continue;
		}

		BlackboardComponent->SetValueAsVector(
			PatrolLocationKey.SelectedKeyName,
			PatrolPoint->GetActorLocation()
		);

		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}