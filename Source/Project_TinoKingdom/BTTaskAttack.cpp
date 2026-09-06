// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTaskAttack.h"

#include "AIController.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"

UBTTaskAttack::UBTTaskAttack()
{
	NodeName = TEXT("Attack");
	bNotifyTick = true;
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

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = BlackboardComponent != nullptr
		? Cast<AActor>(BlackboardComponent->GetValueAsObject(AEnemyAIController::TargetPlayer))
		: nullptr;

	if (TargetActor == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	EnemyCharacter->SetCombatTarget(TargetActor);

	if (!EnemyCharacter->CanAttack())
	{
		return EBTNodeResult::Failed;
	}
	
	if (AEnemyCharacter* TargetEnemy = Cast<AEnemyCharacter>(TargetActor))
	{
		if (TargetEnemy->IsDead())
		{
			EnemyCharacter->SetCombatTarget(nullptr);
			return EBTNodeResult::Failed;
		}
	}
	
	EnemyCharacter->SetCombatTarget(TargetActor);
	
	
	FVector Direction = TargetActor->GetActorLocation() - EnemyCharacter->GetActorLocation();
	Direction.Z = 0.0f;

	if (!Direction.IsNearlyZero())
	{
		const FRotator LookRotation = Direction.Rotation();
		EnemyCharacter->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
	}

	// 이전 Move To 요청을 취소한다. 몽타주의 Root Motion 이동은 영향을 받지 않는다.
	AIController->StopMovement();

	return EnemyCharacter->RequestAttack()
		? EBTNodeResult::InProgress
		: EBTNodeResult::Failed;
}

void UBTTaskAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	AEnemyCharacter* EnemyCharacter = AIController != nullptr
		? Cast<AEnemyCharacter>(AIController->GetPawn())
		: nullptr;

	if (EnemyCharacter == nullptr || EnemyCharacter->IsDead())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!EnemyCharacter->IsAttacking())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
