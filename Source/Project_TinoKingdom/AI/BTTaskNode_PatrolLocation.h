// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BTTaskNode_PatrolLocation.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API UBTTaskNode_PatrolLocation : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTaskNode_PatrolLocation();

	virtual uint16 GetInstanceMemorySize() const override;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PatrolLocationKey;
};
