// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

const FName AEnemyAIController::TargetPlayer(TEXT("TargetPlayer"));
const FName AEnemyAIController::HomeLocation(TEXT("HomeLocation"));

AEnemyAIController::AEnemyAIController()
{
	
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(InPawn);
	if (EnemyCharacter == nullptr)
	{
		return;
	}
	
	UBehaviorTree* BehaviorTree = EnemyCharacter -> GetBehaviorTree();
	if (BehaviorTree == nullptr)
	{
		return;
	}
	
	RunBehaviorTree(BehaviorTree);

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->SetValueAsVector(HomeLocation, InPawn->GetActorLocation());
	}
}

void AEnemyAIController::OnUnPossess()
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->ClearValue(TargetPlayer);
	}

	Super::OnUnPossess();
}

