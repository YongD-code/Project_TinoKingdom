// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TinoAttributeSet.generated.h"

UCLASS()
class PROJECT_TINOKINGDOM_API UTinoAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UTinoAttributeSet();
	
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, Health)
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, MaxHealth)
	
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, Stamina)
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, MaxStamina)
	
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, AttackPower)
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, Defense)

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina")
	FGameplayAttributeData Stamina;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina")
	FGameplayAttributeData MaxStamina;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData AttackPower;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData Defense;
};