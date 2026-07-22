// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Project_TinoKingdom/Interface/CombatAttackAnimationInterface.h"
#include "PlayerCharacter.generated.h"

class UAnimInstance;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UAnimMontage;
class USkeletalMeshComponent;
class UAttackComboData;
class UStatComponent;
class UTinoCombatComponent;
struct FInputActionValue;

UCLASS()
class PROJECT_TINOKINGDOM_API APlayerCharacter : public ACharacter, public ICombatAttackAnimationInterface
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void StartRunning();
	void StopRunning();

	void Attack();

	void StartComboAttack(UAnimInstance* AnimInstance, const UAttackComboData* AttackData);
	void TryQueueNextCombo(UAnimInstance* AnimInstance, const UAttackComboData* AttackData);
	void ResetCombo();
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	const UAttackComboData* GetCurrentAttackData() const;

	void StartJump();
	bool IsAttacking() const;
	int32 FindActiveComboAttackSectionIndex() const;

public:
	UFUNCTION(BlueprintPure, Category = "Stat")
	UStatComponent* GetStatComponent() const { return StatComponent; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	UTinoCombatComponent* GetCombatComponent() const { return CombatComponent; }
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float WalkSpeed = 140.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float RunSpeed = 400.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	TObjectPtr<UStatComponent> StatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UTinoCombatComponent> CombatComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float RunningStamina = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float RecoverStaminaWhileRest = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina")
	float StaminaDelay = 1.5f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> VisibleBodyMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAttackComboData> UnarmedAttackData;

private:
	int32 QueuedComboIndex = INDEX_NONE;
	bool bComboInputWindowOpen = false;
	bool bComboInputConsumed = false;
	bool bRunning = false;

	int32 ActiveAttackSectionIndex = INDEX_NONE;
	bool bAttackHitWindowOpen = false;
	TSet<TWeakObjectPtr<AActor>> HitActorsThisWindow;

	float StaminaDelayTime = 0.0f;

private:
	void PerformAttackTrace();

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetComboInputWindowOpen(bool bIsOpen) override;
	virtual void BeginAttackHitWindow() override;
	virtual void TickAttackHitWindow(float DeltaTime) override;
	virtual void EndAttackHitWindow() override;
};
