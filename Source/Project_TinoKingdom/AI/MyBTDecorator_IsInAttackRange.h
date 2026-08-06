// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "MyBTDecorator_IsInAttackRange.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API UMyBTDecorator_IsInAttackRange : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UMyBTDecorator_IsInAttackRange();

protected:
	virtual bool CalculateRawConditionValue(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) const override;
};
