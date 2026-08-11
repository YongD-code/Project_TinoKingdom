// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTaskAttack.h"

#include "AIController.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"

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
	
	FVector Direction = TargetActor->GetActorLocation() - EnemyCharacter->GetActorLocation();
	Direction.Z = 0.0f;

	if (!Direction.IsNearlyZero())
	{
		const FRotator LookRotation = Direction.Rotation();
		EnemyCharacter->SetActorRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
	}

	return EnemyCharacter->RequestAttack()
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}