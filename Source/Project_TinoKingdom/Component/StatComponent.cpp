// Fill out your copyright notice in the Description page of Project Settings.


#include "StatComponent.h"

UStatComponent::UStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UStatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentHP = MaxHP;
	CurrentStamina = MaxStamina;
	
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UStatComponent::ApplyDamage(float DamageAmount)
{
	if (bDead)
	{
		return;
	}
	
	const float FinalDamage = FMath::Max(DamageAmount-Defense, 1.0f);
	CurrentHP = FMath::Clamp(CurrentHP - FinalDamage, 0.0f, MaxHP);
	
	OnHPChanged.BroadCast(CurrentHP, MaxHP);
	
	if (CurrentHP <= 0.0f)
	{
		bDead = true;
		OnDead.Broadcast();
	}
}

void UStatComponent::Heal(float HealAmount)
{
	if (bDead)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP + HealAmount, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

bool UStatComponent::ConsumeStamina(float StaminaAmount)
{
	if (CurrentStamina < StaminaAmount)
	{
		return false;
	}

	CurrentStamina = FMath::Clamp(CurrentStamina - StaminaAmount, 0.0f, MaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	return true;
}

void UStatComponent::RecoverStamina(float StaminaAmount)
{
	CurrentStamina = FMath::Clamp(CurrentStamina + StaminaAmount, 0.0f, MaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

bool UStatComponent::IsDead() const
{
	return bDead;
}

