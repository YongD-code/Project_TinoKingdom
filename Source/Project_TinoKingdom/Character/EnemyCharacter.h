// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UBehaviorTree;
class UStatComponent;
class UAnimMontage;


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
	bool GetAttackRange() const {return AttackRange;}
	
	UFUNCTION(BlueprintPure, Category = "Enemy | Combat")
	UBehaviorTree* GetBehaviorTree() const {return BehaviorTree;}

protected:
	
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDead();
	
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	TObjectPtr<UStatComponent> StatComponent;
	
	
public:	

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
