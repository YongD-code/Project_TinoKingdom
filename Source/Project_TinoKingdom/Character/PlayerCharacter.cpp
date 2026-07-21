// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/RotationMatrix.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimMontage.h"
#include "Project_TinoKingdom/DataAsset/AttackComboData.h"
#include "Project_TinoKingdom/Component/StatComponent.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	
	MovementComponent->bOrientRotationToMovement = true;
	MovementComponent->RotationRate = FRotator(0.f, 500.f, 0.f);
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	//스탯 컴포넌트 생성자 
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	
	// 플레이어의 움직임 관성 값들
	MovementComponent->MaxAcceleration = 500.f;
	MovementComponent->BrakingDecelerationWalking = 450.f;
	MovementComponent->GroundFriction = 6.f;

	MovementComponent->bUseSeparateBrakingFriction = true;
	MovementComponent->BrakingFriction = 0.4f;
	MovementComponent->BrakingFrictionFactor = 1.f;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	
	static const FName AnimationBodyTag(TEXT("AnimationBody"));
	VisibleBodyMesh = FindComponentByTag<USkeletalMeshComponent>(AnimationBodyTag);
	
	USkeletalMeshComponent* DriverMesh = GetMesh();
	DriverMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	DriverMesh->SetHiddenInGame(true);
	
	// true - bForceUpdate 값인데 true, false 둘 다 상관없을듯. 어차피 BeginPlay()에서 실행하니까
	// fasle - CharacterMesh0의 포즈를 따라가는 Body가 별도로 TickPose를 실행하지 않게 
	VisibleBodyMesh->SetLeaderPoseComponent(DriverMesh, true, false);
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	
	//틱마다 스테미나 감소 계산 체크 
	if (StatComponent == nullptr)
	{
		return;
	}
	
	if (bRunning)
	{
		const bool bConsumedStamina = StatComponent->ConsumeStamina(RunningStamina * DeltaTime);
		StaminaDelayTime = StaminaDelay;
		if (!bConsumedStamina)
		{
			StopRunning();
		}
		return;
	}
	
	if (StaminaDelayTime >= 0.0f)
	{
		StaminaDelayTime -= DeltaTime;
		return;
	}
	
	StatComponent->RecoverStamina(RecoverStaminaWhileRest * DeltaTime);
	

}

void APlayerCharacter::SetComboInputWindowOpen(bool bIsOpen)
{
	bComboInputWindowOpen = bIsOpen;
	if (bIsOpen)
	{
		bComboInputConsumed = false;
	}
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	
	if (EnhancedInputComponent == nullptr)
	{
		return;
	}
	
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::StartJump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Attack);
	if (SprintAction != nullptr)
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::StartRunning);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRunning);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &APlayerCharacter::StopRunning);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// 공격 중에는 이동 입력 액션이 불가능하게
	if (IsAttacking())
	{
		return;
	}
	
	if (Controller == nullptr)
	{
		return;
	}
	
	const FVector2D MovementInput = Value.Get<FVector2D>();
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	AddMovementInput(Forward, MovementInput.Y);
	AddMovementInput(Right, MovementInput.X);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void APlayerCharacter::StartRunning()
{
	if (IsAttacking())
	{
		return;
	}
	
	//스테미나 사용해서 달리기 임시 추가본
	if (StatComponent == nullptr || StatComponent->GetCurrentStamina() <= 0.0f)
	{
		return;
	}
	
	bRunning = true;
	
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void APlayerCharacter::StopRunning()
{
	bRunning = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::Attack()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (bPressedJump || !MovementComponent->IsMovingOnGround())
	{
		return;
	}
	
	const UAttackComboData* AttackData = GetCurrentAttackData();
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	
	if (!IsAttacking())
	{
		StartComboAttack(AnimInstance, AttackData);
		return;
	}
	
	TryQueueNextCombo(AnimInstance, AttackData);
}

void APlayerCharacter::StartComboAttack(UAnimInstance* AnimInstance, const UAttackComboData* AttackData)
{
	// 몽타지 재생
	const float MontageDuration = AnimInstance->Montage_Play(AttackData->AttackMontage);
	if (MontageDuration <= 0.f)
	{
		ResetCombo();
		return;
	}
	
	// 재생 성공 시
	CurrentComboIndex = 0;
	bComboInputWindowOpen = false;
	bComboInputConsumed = false;
	
	// Data Asset의 첫 번째 섹션으로 이동
	AnimInstance->Montage_JumpToSection(AttackData->ComboSectionNames[CurrentComboIndex], AttackData->AttackMontage);
	FOnMontageEnded MontageEndedDelegate;
	
	// 몽타지 종료 델리게이트 등록
	MontageEndedDelegate.BindUObject(this, &APlayerCharacter::OnAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AttackData->AttackMontage);
	
	// 기존 이동 속도 제거
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
}

void APlayerCharacter::TryQueueNextCombo(UAnimInstance* AnimInstance, const UAttackComboData* AttackData)
{
	// 입력 허용 구간이 아니거나 이미 입력을 받았다면 무시
	if (!bComboInputWindowOpen || bComboInputConsumed)
	{
		return;
	}
	
	const int32 NextComboIndex = CurrentComboIndex + 1;
	
	// 마지막 콤보라서 다음 공격이 없다면 무시
	if (!AttackData->ComboSectionNames.IsValidIndex(NextComboIndex))
	{
		return;
	}
	
	const FName CurrentSectionName = AttackData->ComboSectionNames[CurrentComboIndex];
	const FName NextSectionName = AttackData->ComboSectionNames[NextComboIndex];
	
	AnimInstance->Montage_SetNextSection(CurrentSectionName, NextSectionName, AttackData->AttackMontage);
	CurrentComboIndex = NextComboIndex;
	bComboInputConsumed = true;
}

void APlayerCharacter::ResetCombo()
{
	CurrentComboIndex = INDEX_NONE;
	bComboInputWindowOpen = false;
	bComboInputConsumed = false;
}

void APlayerCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ResetCombo();
}

const UAttackComboData* APlayerCharacter::GetCurrentAttackData() const
{
	return UnarmedAttackData.Get();
}

void APlayerCharacter::StartJump()
{
	if (IsAttacking())
	{
		return;
	}
	Jump();
}

bool APlayerCharacter::IsAttacking() const
{
	// 이제는 몽타지 직접 검사할 필요 X
	return CurrentComboIndex != INDEX_NONE;
}
