// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TinoAttributeSet.generated.h"

struct FGameplayEffectModCallbackData;

UCLASS()
class PROJECT_TINOKINGDOM_API UTinoAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UTinoAttributeSet();
	
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, Health)
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, MaxHealth)
	
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, Stamina)
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, MaxStamina)
	
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, AttackPower)
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, Defense)
	
	ATTRIBUTE_ACCESSORS_BASIC(UTinoAttributeSet, IncomingDamage)

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
	
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	
private:
	void ClampAttributeValue(const FGameplayAttribute& Attribute, float& NewValue) const;
};