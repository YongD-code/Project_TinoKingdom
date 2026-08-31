// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Project_TinoKingdom/Interface/TargetableInterface.h"
#include "EnemyCharacter.generated.h"

class UBehaviorTree;	
class UStatComponent;
class UAnimMontage;
class AEnemyAIController;
class ATargetPoint;
class APlayerCharacter;
class UTexture2D;

UCLASS()
class PROJECT_TINOKINGDOM_API AEnemyCharacter : public ACharacter, public ITargetableInterface
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
	
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void SetAggroTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	bool IsDead() const { return bDead; }

	void PlayHitReaction();
	
	void SetCombatTarget(AActor* Target);
	
	void ApplyKnockbackFrom(AActor* DamageCauser);
	
	const TArray<TObjectPtr<ATargetPoint>>& GetPatrolPoints() const {return PatrolPoints;}
	
	virtual bool CanBeTargeted_Implementation() const override;
	virtual FVector GetLockOnLocation_Implementation() const override;

protected:
	
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleDead();
	
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	TObjectPtr<UStatComponent> StatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting")
	TObjectPtr<USceneComponent> LockOnAnchor;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Enemy|AI")
	TArray<TObjectPtr<ATargetPoint>> PatrolPoints;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UAnimMontage> HitMontage;
    	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death")
	TObjectPtr<UAnimMontage> DeathMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop")
	int32 DropExperience = 50;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop")
	FName DropItemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop")
	FText DropItemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop", meta = (ClampMin = "0"))
	int32 DropItemCount = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop")
	TObjectPtr<UTexture2D> DropItemIcon = nullptr;

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
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> CombatTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "0.0"))
	float AttackTurnSpeed = 1440.0f;
	
	UPROPERTY(Transient)
	bool bHitReacting = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Hit", meta = (ClampMin = "0.0"))
	float KnockbackPower = 250.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Hit", meta = (ClampMin = "0.0"))
	float KnockbackUpPower = 80.0f;
	

	
	
	virtual void Tick(float DeltaSeconds) override;
private:
	UPROPERTY(Transient)
	bool bAttacking = false;
	
	UPROPERTY(Transient)
	bool bDead = false;
	
	UPROPERTY(Transient)
	float LastAttackTime = -999.f;
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> LastDamageCauser;
	
	FTimerHandle AttackResetTimerHandle;
	FTimerHandle HitReactionResetTimerHandle;

	void ResetAttackState();
	void ResetHitReactionState();
};
