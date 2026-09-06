// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBTDecorator_IsInAttackRange.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

UMyBTDecorator_IsInAttackRange::UMyBTDecorator_IsInAttackRange()
{
	NodeName = TEXT("Is In Attack Range");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
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

	return EnemyCharacter->IsTargetWithinAttackRange(TargetActor);
}

void UMyBTDecorator_IsInAttackRange::OnBecomeRelevant(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	TNodeInstanceMemory* DecoratorMemory = CastInstanceNodeMemory<TNodeInstanceMemory>(NodeMemory);
	DecoratorMemory->bLastRawResult = CalculateRawConditionValue(OwnerComp, NodeMemory);
}

void UMyBTDecorator_IsInAttackRange::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	TNodeInstanceMemory* DecoratorMemory = CastInstanceNodeMemory<TNodeInstanceMemory>(NodeMemory);
	const bool bCurrentRawResult = CalculateRawConditionValue(OwnerComp, NodeMemory);

	if (bCurrentRawResult != DecoratorMemory->bLastRawResult)
	{
		DecoratorMemory->bLastRawResult = bCurrentRawResult;
		OwnerComp.RequestExecution(this);
	}
}

uint16 UMyBTDecorator_IsInAttackRange::GetInstanceMemorySize() const
{
	return sizeof(TNodeInstanceMemory);
}

void UMyBTDecorator_IsInAttackRange::InitializeMemory(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<TNodeInstanceMemory>(NodeMemory, InitType);
}

void UMyBTDecorator_IsInAttackRange::CleanupMemory(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<TNodeInstanceMemory>(NodeMemory, CleanupType);
}

