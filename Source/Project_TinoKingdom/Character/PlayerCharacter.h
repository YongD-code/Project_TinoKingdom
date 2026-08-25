// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemInterface.h"
#include "PlayerCharacter.generated.h"

class UGameplayEffect;
class UTinoAttributeSet;
class UAbilitySystemComponent;
class UTargetingComponent;
class UDodgeComponent;
class UReactionComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class USkeletalMeshComponent;
class UStatComponent;
class UTinoCombatComponent;
class UTinoEquipmentComponent;
class UEquipmentLoadoutData;
class UInventoryComponent;
class ATinoNPCCharacter;
class UTinoStateComponent;
class UTinoAbilitySystemComponent;
struct FInputActionValue;

UCLASS()
class PROJECT_TINOKINGDOM_API APlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UTinoAbilitySystemComponent* GetTinoAbilitySystemComponent() const { return AbilitySystemComponent; }
	
	const UTinoAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void StartRunning();
	void StopRunning();

	ATinoNPCCharacter* FindNearbyNPC() const;
	void Attack();
	void StartJump();
	void MoveDebugFlyUp();

	void ToggleDebugFly();
	void Dodge();

	void StartAiming();
	void StopAiming();
	void RequestTargeting();
	void UpdateCameraTransition(float DeltaTime);

	bool ShouldUseStrafeMovement() const;
	void UpdateRotationMode();
	void UpdateMovementSpeed();
	void UpdateLockOnCamera(float DeltaTime);

	UPROPERTY()
	AActor* PreviousViewTarget;

	UPROPERTY()
	APlayerController* DialoguePlayerController;

public:
	UFUNCTION(BlueprintPure, Category = "Stat")
	UStatComponent* GetStatComponent() const { return StatComponent; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UTinoAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UTinoAttributeSet> AttributeSet;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability System")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability System")
	TSubclassOf<UGameplayEffect> DamageEffect;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Aim", meta = (ClampMin = "0.0", Units = "cm"))
	float AimCameraArmLength = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Aim", meta = (Units = "cm"))
	FVector AimCameraSocketOffset = FVector(0.0f, 65.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Aim", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
	float AimFieldOfView = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Aim", meta = (ClampMin = "0.0"))
	float AimCameraInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (ClampMin = "0.0", Units = "cm"))
	float LockOnCameraArmLength = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (Units = "cm"))
	FVector LockOnCameraSocketOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (Units = "cm"))
	FVector LockOnCameraTargetOffset = FVector(0.f, 0.f, 100.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (Units = "cm"))
	FVector LockOnFollowCameraRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (Units = "deg"))
	FRotator LockOnFollowCameraRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
	float LockOnFieldOfView = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (ClampMin = "0.0"))
	float LockOnCameraLagSpeed = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (ClampMin = "0.0", Units = "cm"))
	float LockOnCameraLagMaxDistance = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (ClampMin = "0.0"))
	float LockOnCameraInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (ClampMin = "-89.0", ClampMax = "89.0", Units = "deg"))
	float LockOnCameraMinPitch = -40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lock On", meta = (ClampMin = "-89.0", ClampMax = "89.0", Units = "deg"))
	float LockOnCameraMaxPitch = 45.f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleEquipmentMenuAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Debug")
	TObjectPtr<UInputAction> ToggleDebugAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> TargetingAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float WalkSpeed = 140.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float StrafeSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float RunSpeed = 400.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UTinoCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTinoEquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reaction")
	TObjectPtr<UReactionComponent> ReactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	TObjectPtr<UTinoStateComponent> CharacterStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<UDodgeComponent> DodgeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting")
	TObjectPtr<UTargetingComponent> TargetingComponent;

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void OpenEquipmentWheel();

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void ConfirmEquipmentWheel();

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void CancelEquipmentWheel();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Equipment|Time")
	void StartSlowMotion();

	UFUNCTION(BlueprintCallable, Category = "Equipment|Time")
	void StopSlowMotion();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Time")
	float TimeDilation = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float RunningStamina = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float RecoverStaminaWhileRest = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina")
	float StaminaDelay = 1.5f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> VisibleBodyMesh;

private:
	UPROPERTY(Transient)
	float SavedGlobalTimeDilation = 1.f;

	UPROPERTY(Transient)
	bool bEquipmentWheelSlowMotionActive = false;

	bool bRunning = false;
	float StaminaDelayTime = 0.0f;

	bool bDeathHandled = false;

	bool bIsAiming = false;
	bool bCameraTransition = false;

	float DefaultCameraArmLength = 0.0f;
	FVector DefaultCameraSocketOffset = FVector::ZeroVector;
	FVector DefaultCameraTargetOffset = FVector::ZeroVector;
	FVector DefaultFollowCameraRelativeLocation = FVector::ZeroVector;
	FRotator DefaultFollowCameraRelativeRotation = FRotator::ZeroRotator;
	float DefaultCameraFieldOfView = 90.0f;
	float DefaultCameraLagSpeed = 10.f;
	float DefaultCameraLagMaxDistance = 40.f;

private:
	bool InitializeDefaultAttributes();
	
	float ApplyDamageGameplayEffect(float DamageAmount, AController* EventInstigator, AActor* DamageCauser);
	
	UFUNCTION()
	void HandleEquipmentChanged(UEquipmentLoadoutData* NewLoadout);

	UFUNCTION()
	void HandleDeath(AActor* DamageCauser);

	UFUNCTION()
	void HandleLockOnTargetChanged(AActor* PreviousTarget, AActor* NewTarget);
};
