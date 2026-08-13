// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_UpdateTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = TEXT("Update Target");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
}

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory,float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* OwnerPawn = AIController != nullptr ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (OwnerPawn == nullptr || BlackboardComponent == nullptr)
	{
		return;
	}

	const FVector OwnerLocation = OwnerPawn->GetActorLocation();
	const FVector HomeLocation = BlackboardComponent->GetValueAsVector(
		AEnemyAIController::HomeLocation
	);

	AActor* CurrentTarget = Cast<AActor>(
		BlackboardComponent->GetValueAsObject(AEnemyAIController::TargetPlayer)
	);

	if (CurrentTarget != nullptr)
	{
		AEnemyCharacter* OwnerEnemy = Cast<AEnemyCharacter>(OwnerPawn);
		AEnemyCharacter* TargetEnemy = Cast<AEnemyCharacter>(CurrentTarget);

		if (TargetEnemy != nullptr && TargetEnemy->IsDead())
		{
			BlackboardComponent->ClearValue(AEnemyAIController::TargetPlayer);

			if (OwnerEnemy != nullptr)
			{
				OwnerEnemy->SetCombatTarget(nullptr);
			}

			return;
		}

		const float DistanceFromHome = FVector::Dist2D(
			OwnerLocation,
			HomeLocation
		);

		const float DistanceToCurrentTarget = FVector::Dist2D(
			OwnerLocation,
			CurrentTarget->GetActorLocation()
		);

		if (DistanceFromHome > LeashRadius || DistanceToCurrentTarget > LoseRadius)
		{
			BlackboardComponent->ClearValue(AEnemyAIController::TargetPlayer);

			if (OwnerEnemy != nullptr)
			{
				OwnerEnemy->SetCombatTarget(nullptr);
			}
		}

		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(OwnerPawn, 0);
	if (PlayerPawn == nullptr)
	{
		return;
	}

	const float DistanceToPlayer = FVector::Dist2D(
		OwnerLocation,
		PlayerPawn->GetActorLocation()
	);

	if (DistanceToPlayer <= DetectRadius)
	{
		BlackboardComponent->SetValueAsObject(
			AEnemyAIController::TargetPlayer,
			PlayerPawn
		);
	}
}