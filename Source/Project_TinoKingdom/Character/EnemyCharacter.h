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
class UEnemyHealthBarWidget;
class UTexture2D;
class UWidgetComponent;

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

	UFUNCTION()
	void HandleHPChanged(float CurrentValue, float MaxValue);
	
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void UpdateHealthBar(float CurrentValue, float MaxValue);
	void UpdateHealthBarVisibility();
	bool IsHealthBarInVisibleRange() const;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	TObjectPtr<UStatComponent> StatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting")
	TObjectPtr<USceneComponent> LockOnAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Enemy|AI")
	TArray<TObjectPtr<ATargetPoint>> PatrolPoints;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 180.0f;

	// 플레이어가 이 거리 안에 있고 지면 충돌이 준비됐을 때만 AI와 이동을 활성화한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float AIActivationDistance = 10000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI", meta = (ClampMin = "0.05"))
	float AIActivationCheckInterval = 0.25f;

	// 스트리밍 중 이 거리보다 아래로 떨어지면 마지막으로 확인된 안전 위치로 복구한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI", meta = (ClampMin = "100.0"))
	float MaxStreamingFallDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI", meta = (ClampMin = "0.0"))
	float GroundProbeDistance = 200.0f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|UI", meta = (ClampMin = "0.0"))
	float HealthBarHeight = 110.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|UI", meta = (ClampMin = "1.0"))
	FVector2D HealthBarDrawSize = FVector2D(90.0f, 12.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|UI", meta = (ClampMin = "0.0"))
	float MaxHealthBarVisibleDistance = 1800.0f;
	

	
	
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
	FTimerHandle AIActivationTimerHandle;

	FTransform LastSafeTransform = FTransform::Identity;

	bool bAIActive = false;

	void ResetAttackState();
	void ResetHitReactionState();
	void UpdateAIActivation();
	void SetEnemyAIActive(bool bEnabled);
	bool HasGroundBelow(const FVector& Location) const;
};
