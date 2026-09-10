// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "Engine/World.h"
#include "Project_TinoKingdom/World/TinoEndingCrowdSubsystem.h"

const FName AEnemyAIController::TargetPlayer(TEXT("TargetPlayer"));
const FName AEnemyAIController::HomeLocation(TEXT("HomeLocation"));
const FName AEnemyAIController::PatrolLocation(TEXT("PatrolLocation"));

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
	// 소유 처리가 몬스터의 플레이 시작보다 먼저 발생해도 엔딩 이후 전투 트리를 실행하지 않습니다.
	const UTinoEndingCrowdSubsystem* EndingCrowd = GetWorld()->GetSubsystem<UTinoEndingCrowdSubsystem>();
	if (EnemyCharacter->IsCinematicAIBlocked() || (EndingCrowd && EndingCrowd->ShouldConvert(*EnemyCharacter)))
	{
		return;
	}
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

