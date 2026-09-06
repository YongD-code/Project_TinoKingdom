// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTaskAttack.generated.h"

struct FAttackTaskMemory
{
	bool bAttackStarted = false;
};

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API UBTTaskAttack : public UBTTaskNode
{
	GENERATED_BODY()

	typedef FAttackTaskMemory TNodeInstanceMemory;
	
public:
	UBTTaskAttack();
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
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
