// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBTDecorator_IsFarFromHome.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"

UMyBTDecorator_IsFarFromHome::UMyBTDecorator_IsFarFromHome()
{
	NodeName = TEXT("Is Far From Home");
}

bool UMyBTDecorator_IsFarFromHome::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* OwnerPawn = AIController != nullptr ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (OwnerPawn == nullptr || BlackboardComponent == nullptr)
	{
		return false;
	}

	const FVector HomeLocation = BlackboardComponent->GetValueAsVector(AEnemyAIController::HomeLocation);

	const float DistanceFromHome = FVector::Dist2D(
		OwnerPawn->GetActorLocation(),
		HomeLocation
	);

	return DistanceFromHome > ReturnDistance;
}