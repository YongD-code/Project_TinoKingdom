// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Math/RotationMatrix.h"
#include "Project_TinoKingdom/DataAsset/AttackComboData.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoCombat, Log, All);

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
	
	// 플레이어의 움직임 관성 값들
	MovementComponent->MaxAcceleration = 500.f;
	MovementComponent->BrakingDecelerationWalking = 450.f;
	MovementComponent->GroundFriction = 6.f;

	MovementComponent->bUseSeparateBrakingFriction = true;
	MovementComponent->BrakingFriction = 0.4f;
	MovementComponent->BrakingFrictionFactor = 1.f;
	
	// 충돌 프리셋 적용
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("TinoCapsule"));
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
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void APlayerCharacter::StopRunning()
{
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
	if (!IsValid(AnimInstance))
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: AnimInstance가 없습니다."), *GetNameSafe(this));
		return;
	}

	if (!IsValid(AttackData))
	{
		UE_LOG(LogTinoCombat,Error, TEXT("%s: AttackData가 지정되지 않았습니다."), *GetNameSafe(this));
		return;
	}

	if (!IsValid(AttackData->AttackMontage))
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: AttackMontage가 지정되지 않았습니다."), *GetNameSafe(this));
		return;
	}

	if (!AttackData->ComboSection.IsValidIndex(0))
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: ComboSection 배열이 비어 있습니다."), *GetNameSafe(this));
		return;
	}

	const FName FirstSectionName = AttackData->ComboSection[0].SectionName;
	if (!AttackData->AttackMontage->IsValidSectionName(FirstSectionName))
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: 몽타주 %s에 섹션 %s이 존재하지 않습니다."),
			*GetNameSafe(this), *GetNameSafe(AttackData->AttackMontage), *FirstSectionName.ToString());
		return;
	}

	const float MontageDuration = AnimInstance->Montage_Play(AttackData->AttackMontage);
	if (MontageDuration <= 0.f)
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: 공격 몽타주 %s 재생에 실패했습니다."),
			*GetNameSafe(this), *GetNameSafe(AttackData->AttackMontage));
		ResetCombo();
		return;
	}
	
	// 재생 성공 시
	QueuedComboIndex = 0;
	bComboInputWindowOpen = false;
	bComboInputConsumed = false;
	
	// Data Asset의 첫 번째 섹션으로 이동
	AnimInstance->Montage_JumpToSection(FirstSectionName, AttackData->AttackMontage);
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
	
	const int32 NextComboIndex = QueuedComboIndex + 1;
	
	// 마지막 콤보라서 다음 공격이 없다면 무시
	if (!AttackData->ComboSection.IsValidIndex(NextComboIndex))
	{
		return;
	}
	
	const FName CurrentSectionName = AttackData->ComboSection[QueuedComboIndex].SectionName;
	const FName NextSectionName = AttackData->ComboSection[NextComboIndex].SectionName;
	
	AnimInstance->Montage_SetNextSection(CurrentSectionName, NextSectionName, AttackData->AttackMontage);
	QueuedComboIndex = NextComboIndex;
	bComboInputConsumed = true;
}

void APlayerCharacter::ResetCombo()
{
	QueuedComboIndex = INDEX_NONE;
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
	return QueuedComboIndex != INDEX_NONE;
}
