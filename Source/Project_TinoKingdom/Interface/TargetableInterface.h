// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UTargetableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECT_TINOKINGDOM_API ITargetableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Targeting")
	bool CanBeTargeted() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Targeting")
	FVector GetLockOnLocation() const;
};
