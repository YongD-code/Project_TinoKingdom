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
struct FInputActionValue;

UCLASS()
class PROJECT_TINOKINGDOM_API APlayerCharacter : public ACharacter, public ICombatAttackAnimationInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called to bind functionality to input
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
	
	//스탯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	TObjectPtr<UStatComponent> StatComponent; 
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	//1초당 감소 스테미나 설정 (차차 조절하면 될듯)
	float RunningStamina = 15.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	//스테미나를 소모하지 않는 상태일때 스테미나 회복량
	float RecoverStaminaWhileRest = 20.0f;
	
// Animation Montage
protected:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> VisibleBodyMesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAttackComboData> UnarmedAttackData;
	
private:
	uint32 CurrentComboIndex = INDEX_NONE;
	uint8 bComboInputWindowOpen : 1 = false;
	uint8 bComboInputConsumed : 1 = false;
	uint8 bRunning : 1 = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetComboInputWindowOpen(bool bIsOpen) override;
};
