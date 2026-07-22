// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttackComboData.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FComboAttackSectionData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName SectionName = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Damage = 10.f;
	
	// 0은 대상 수 제한 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "1"))
	int32 MaxHitTargets = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
	float TraceStartForwardOffset = 20.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace", meta = (ClampMin = "0.0"))
	float TraceDistance = 100.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
	float TraceHeightOffset = 30.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace", meta = (ClampMin = "1.0"))
	float TraceRadius = 35.f;
};

class UAnimMontage;

UCLASS(BlueprintType)
class PROJECT_TINOKINGDOM_API UAttackComboData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TArray<FComboAttackSectionData> ComboSection;
};
