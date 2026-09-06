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
	TNodeInstanceMemory* TaskMemory = CastInstanceNodeMemory<TNodeInstanceMemory>(NodeMemory);
	TaskMemory->bAttackStarted = false;

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

	// 사거리 안에서 쿨다운이 끝날 때까지 대기한다. 실패 처리하면 Move To가 다시 실행되어
	// DashAttack 거리 구간을 그대로 통과할 수 있다.
	if (!EnemyCharacter->CanAttack())
	{
		return EBTNodeResult::InProgress;
	}

	TaskMemory->bAttackStarted = EnemyCharacter->RequestAttack();
	return TaskMemory->bAttackStarted ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

void UBTTaskAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	TNodeInstanceMemory* TaskMemory = CastInstanceNodeMemory<TNodeInstanceMemory>(NodeMemory);

	AAIController* AIController = OwnerComp.GetAIOwner();
	AEnemyCharacter* EnemyCharacter = AIController != nullptr
		? Cast<AEnemyCharacter>(AIController->GetPawn())
		: nullptr;

	if (EnemyCharacter == nullptr || EnemyCharacter->IsDead())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = BlackboardComponent != nullptr
		? Cast<AActor>(BlackboardComponent->GetValueAsObject(AEnemyAIController::TargetPlayer))
		: nullptr;

	if (!IsValid(TargetActor))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!TaskMemory->bAttackStarted)
	{
		if (!EnemyCharacter->IsTargetWithinAttackRange(TargetActor))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		EnemyCharacter->SetCombatTarget(TargetActor);
		AIController->StopMovement();

		FVector Direction = TargetActor->GetActorLocation() - EnemyCharacter->GetActorLocation();
		Direction.Z = 0.0f;
		if (!Direction.IsNearlyZero())
		{
			const FRotator LookRotation = Direction.Rotation();
			EnemyCharacter->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
		}

		if (EnemyCharacter->CanAttack())
		{
			TaskMemory->bAttackStarted = EnemyCharacter->RequestAttack();
			if (!TaskMemory->bAttackStarted)
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			}
		}

		return;
	}

	if (!EnemyCharacter->IsAttacking())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UBTTaskAttack::GetInstanceMemorySize() const
{
	return sizeof(TNodeInstanceMemory);
}

void UBTTaskAttack::InitializeMemory(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<TNodeInstanceMemory>(NodeMemory, InitType);
}

void UBTTaskAttack::CleanupMemory(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<TNodeInstanceMemory>(NodeMemory, CleanupType);
}
