// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatValueChanged,float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDead);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UStatComponent();

protected:

	virtual void BeginPlay() override;
	
public:
	UFUNCTION(BlueprintCallable,Category= "Stat")
	void ApplyDamage(float DamageAmount);
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	void Heal(float HealAmount);
		
	UFUNCTION(BlueprintCallable,Category= "Stat")
	bool ConsumeStamina(float StaminaAmount);
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	void RecoverStamina(float StaminaAmount);
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	bool IsDead() const;
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	float GetCurrentHP() const {return CurrentHP();}
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	float GetMaxHP() const {return MaxHP();}
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	float GetCurrentStamina() const {return CurrentStamina();}
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	float GetMaxStamina() const {return MaxStamina();}
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	float GetAttackPower() const {return AttackPower();}
	
	UFUNCTION(BlueprintCallable,Category= "Stat")
	float GetDefense() const {return Defense();}
	
public:
	UPROPERTY(BlueprintAssignable,Category= "Stat")
	FOnStatValueChanged OnHPChanged;
	
	UPROPERTY(BlueprintAssignable,Category= "Stat")
	FOnStatValueChanged OnStaminaChanged;
	
	UPROPERTY(BlueprintAssignable,Category= "Stat")
	FOnDead OnDead;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat|HP", meta = (ClampMin = "1.0"))
	float MaxHP = 100.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|HP")
	float CurrentHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat|Stamina", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Stamina")
	float CurrentStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat|Combat", meta = (ClampMin = "0.0"))
	float AttackPower = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat|Combat", meta = (ClampMin = "0.0"))
	float Defense = 2.f;

private:
	bool bDead = false;
	
};
