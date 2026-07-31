// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	static const FName TargetPlayer;
	static const FName HomeLocation;
	
	AEnemyAIController();
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
};
