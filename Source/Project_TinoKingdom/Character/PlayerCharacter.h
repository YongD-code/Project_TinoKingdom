// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Camera/CameraComponent.h"
#include "PlayerCharacter.generated.h"

class UDialogueComponent;
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
struct FInputActionValue;

UCLASS()
class PROJECT_TINOKINGDOM_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();
	
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
	void Interact();
	void DialogueAdvancePressed();
	void DialogueAdvanceReleased();
	void DialogueCancel();
	void Attack();
	void StartJump();
	void MoveDebugFlyUp();
	
	void ToggleDebugFly();
	void Dodge();

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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleEquipmentMenuAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Dialogue")
	TObjectPtr<UInputAction> DialInteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Dialogue")
	TObjectPtr<UInputAction> DialAdvanceAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Dialogue")
	TObjectPtr<UInputAction> DialCancelAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Debug")
	TObjectPtr<UInputAction> ToggleDebugAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bDrawInteractionDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float WalkSpeed = 140.f;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueComponent> DialogueComponent;
	
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

private:
	UFUNCTION()
	void HandleEquipmentChanged(UEquipmentLoadoutData* NewLoadout);
	
	UFUNCTION()
	void HandleDeath(AActor* DamageCauser);
};
