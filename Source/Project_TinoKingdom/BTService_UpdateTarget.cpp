// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_UpdateTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"

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

	AActor* CurrentTarget = Cast<AActor>(
		BlackboardComponent->GetValueAsObject(AEnemyAIController::TargetPlayer)
	);

	if (CurrentTarget != nullptr)
	{
		const float DistanceToCurrentTarget = FVector::Dist(
			OwnerPawn->GetActorLocation(),
			CurrentTarget->GetActorLocation()
		);

		if (DistanceToCurrentTarget <= LoseRadius)
		{
			return;
		}

		BlackboardComponent->ClearValue(AEnemyAIController::TargetPlayer);
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(OwnerPawn, 0);
	if (PlayerPawn == nullptr)
	{
		return;
	}

	const float DistanceToPlayer = FVector::Dist(
		OwnerPawn->GetActorLocation(),
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