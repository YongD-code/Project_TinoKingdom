// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "PlayerCharacter.generated.h"

class UDialogueComponent;
class UCookingRecipeBookComponent;
class UGameplayEffect;
class UTinoAttributeSet;
class UAbilitySystemComponent;
class UPlayerProgressionComponent;
class UTargetingComponent;
class UDodgeComponent;
class UQuestComponent;
class USoundBase;
class UReactionComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class USkeletalMeshComponent;
class UTinoCombatComponent;
class UTinoEquipmentComponent;
class UEquipmentLoadoutData;
class UInventoryComponent;
class UCookingComponent;
class ATinoNPCCharacter;
class UTinoStateComponent;
class UTinoAbilitySystemComponent;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
struct FInputActionValue;

UCLASS()
class PROJECT_TINOKINGDOM_API APlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UTinoAbilitySystemComponent* GetTinoAbilitySystemComponent() const { return AbilitySystemComponent; }
	UPlayerProgressionComponent* GetProgressionComponent() const { return ProgressionComponent; }
	const UTinoAttributeSet* GetAttributeSet() const { return AttributeSet; }
	UTinoAttributeSet* GetMutableAttributeSet() { return AttributeSet; }
	UTinoEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }
	UQuestComponent* GetQuestComponent() const { return QuestComponent; }
	
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
	
	void ToggleCharacterMenu();

	void StartRunning();
	void StopRunning();

	ATinoNPCCharacter* FindNearbyNPC() const;
	void Interact();
	void DialogueAdvancePressed();
	void DialogueAdvanceReleased();

	void Attack();
	void StartJump();
	void MoveDebugFlyUp();

	void ToggleDebugFly();
	void Dodge();

	void StartAiming();
	void StopAiming();
	void RequestTargeting();
	void OpenSecretPlace();
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
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Cooking")
	UCookingComponent* GetCookingComponent() const { return CookingComponent; }

	UFUNCTION(BlueprintPure, Category = "Cooking")
	UCookingRecipeBookComponent* GetCookingRecipeBookComponent() const { return CookingRecipeBookComponent; }

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void ToggleCookingMenu();

	UFUNCTION(BlueprintPure, Category = "Cooking")
	bool IsNearCookingPot() const;
	// 인벤토리 UI에서 Usable 아이템의 실제 효과를 요청한다.
	// SecretPlaceMap/SecretPlaceKey의 소모 여부는 SecretPlaceEntrance가 결정한다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryUseUsableItem(FName ItemId);

	// 비밀 장소 입구와 발표용 직접 입력이 공통으로 사용하는 레벨 이동 함수다.
	UFUNCTION(BlueprintCallable, Category = "Level Travel")
	bool TryOpenSecretPlace();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UTinoAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UPlayerProgressionComponent> ProgressionComponent;
	
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
	TObjectPtr<UInputAction> ToggleCharacterMenuAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Dialogue")
	TObjectPtr<UInputAction> DialInteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Dialogue")
	TObjectPtr<UInputAction> DialAdvanceAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Debug")
	TObjectPtr<UInputAction> ToggleDebugAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> TargetingAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Level Travel")
	TObjectPtr<UInputAction> OpenSecretPlaceAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Travel")
	FName SecretPlaceLevelName = TEXT("/Game/MedievalDungeon/Maps/SecretPlace");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionRadius = 300.f;

	// 플레이어가 피해를 입었을 때 재생할 소리.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> DamagedSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bDrawInteractionDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float WalkSpeed = 140.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float StrafeSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float RunSpeed = 400.f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UTinoCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTinoEquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reaction")
	TObjectPtr<UReactionComponent> ReactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking")
	TObjectPtr<UCookingComponent> CookingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking")
	TObjectPtr<UCookingRecipeBookComponent> CookingRecipeBookComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Interaction", meta = (ClampMin = "0.0"))
	float CookingPotInteractionRadius = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Interaction")
	FName CookingPotActorTag = TEXT("CookingPot");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Interaction")
	FString CookingPotClassNameKeyword = TEXT("CookingPot");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	TObjectPtr<UTinoStateComponent> CharacterStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<UDodgeComponent> DodgeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting")
	TObjectPtr<UTargetingComponent> TargetingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueComponent> DialogueComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	TObjectPtr<UQuestComponent> QuestComponent;

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void OpenEquipmentWheel();

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void ConfirmEquipmentWheel();

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void CancelEquipmentWheel();

	UFUNCTION(BlueprintCallable, Category = "Equipment|Time")
	void StartSlowMotion();

	UFUNCTION(BlueprintCallable, Category = "Equipment|Time")
	void StopSlowMotion();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Time")
	float TimeDilation = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float RunningStamina = 5.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float DodgeStamina = 10.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float JumpStamina = 5.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina", meta = (ClampMin = "0.0"))
	float RecoverStaminaWhileRest = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Stamina")
	float StaminaDelay = 1.5f;

	// 프롤로그 종료 후 검은 화면에서 플레이어 화면으로 전환되는 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Startup", meta = (ClampMin = "0.0"))
	float StartupFadeInDuration = 3.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Respawn")
	float RespawnDelay = 4.f;

	// 런타임에 직접 재생할 부활 카메라/사운드 시퀀스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Respawn|Cinematic")
	TObjectPtr<ULevelSequence> RespawnSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Respawn|Cinematic", meta = (ClampMin = "0.0"))
	float RespawnFadeInDuration = 1.f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> VisibleBodyMesh;

private:
	FTransform InitialSpawnTransform = FTransform::Identity;
	
	UPROPERTY(Transient)
	float SavedGlobalTimeDilation = 1.f;

	UPROPERTY(Transient)
	bool bEquipmentWheelSlowMotionActive = false;

	bool bRunning = false;
	float StaminaDelayTime = 0.0f;

	FTimerHandle RespawnTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> RespawnSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> RespawnSequenceActor;
	
	bool bDeathHandled = false;
	bool bLevelTravelInProgress = false;

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

	void RespawnAtInitialTransform();
	void FinishRespawn(bool bFadeInFromBlack);
	void ClearRespawnSequence();

	UFUNCTION()
	void HandleRespawnSequenceFinished();
	
	float ApplyDamageGameplayEffect(float DamageAmount, AController* EventInstigator, AActor* DamageCauser);
	
	UFUNCTION()
	void HandleEquipmentChanged(UEquipmentLoadoutData* NewLoadout);

	UFUNCTION()
	void HandleDeath(AActor* DamageCauser);

	UFUNCTION()
	void HandleLockOnTargetChanged(AActor* PreviousTarget, AActor* NewTarget);
};
