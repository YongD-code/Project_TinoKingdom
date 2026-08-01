// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UBehaviorTree;
class UStatComponent;
class UAnimMontage;
class AEnemyAIController;

UCLASS()
class PROJECT_TINOKINGDOM_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;
	
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	bool RequestAttack();
	
	UFUNCTION(BlueprintPure, Category= "Enemy|Combat")
	bool CanAttack() const;
	
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool IsAttacking() const {return bAttacking;}
	
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetAttackRange() const {return AttackRange;}
	
	UFUNCTION(BlueprintPure, Category = "Enemy | Combat")
	UBehaviorTree* GetBehaviorTree() const {return BehaviorTree;}
	
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	void PerformAttackTrace();

	void PlayHitReaction();

protected:
	
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDead();
	
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	TObjectPtr<UStatComponent> StatComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UAnimMontage> HitMontage;
    	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float AttackCooldown = 2.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death", meta = (ClampMin = "0.0"))
	float DeadLifeSpan = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float AttackTraceRadius = 50.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float AttackTraceDistance = 120.f;
	
private:
	UPROPERTY(Transient)
	bool bAttacking = false;
	
	UPROPERTY(Transient)
	bool bDead = false;
	
	UPROPERTY(Transient)
	float LastAttackTime = -999.f;
};
