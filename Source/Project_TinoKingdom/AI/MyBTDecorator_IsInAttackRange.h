// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BTDecorator.h"
#include "MyBTDecorator_IsInAttackRange.generated.h"

struct FAttackRangeDecoratorMemory
{
	bool bLastRawResult = false;
};

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API UMyBTDecorator_IsInAttackRange : public UBTDecorator
{
	GENERATED_BODY()

	typedef FAttackRangeDecoratorMemory TNodeInstanceMemory;
	
public:
	UMyBTDecorator_IsInAttackRange();
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTMemoryInit::Type InitType
	) const override;
	virtual void CleanupMemory(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTMemoryClear::Type CleanupType
	) const override;

protected:
	virtual bool CalculateRawConditionValue(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds
	) override;
};
