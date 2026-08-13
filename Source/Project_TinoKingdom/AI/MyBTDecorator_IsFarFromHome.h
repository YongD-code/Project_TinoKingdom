// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "MyBTDecorator_IsFarFromHome.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API UMyBTDecorator_IsFarFromHome : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UMyBTDecorator_IsFarFromHome();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0.0"))
	float ReturnDistance = 150.0f;
};
