// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Math/RotationMatrix.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Project_TinoKingdom/Component/TinoCombatComponent.h"
#include "Project_TinoKingdom/Component/TinoEquipmentComponent.h"

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

	// 스탯 컴포넌트를 기본 서브오브젝트로 생성한다.
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	CombatComponent = CreateDefaultSubobject<UTinoCombatComponent>(TEXT("CombatComponent"));
	EquipmentComponent = CreateDefaultSubobject<UTinoEquipmentComponent>(TEXT("EquipmentComponent"));
	
	// 플레이어 이동의 가속, 감속 및 마찰 값을 설정한다.
	MovementComponent->MaxAcceleration = 500.f;
	MovementComponent->BrakingDecelerationWalking = 450.f;
	MovementComponent->GroundFriction = 6.f;

	MovementComponent->bUseSeparateBrakingFriction = true;
	MovementComponent->BrakingFriction = 0.4f;
	MovementComponent->BrakingFrictionFactor = 1.f;

	// 플레이어 전용 충돌 프리셋을 적용한다.
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

	// 숨겨진 Driver Mesh의 포즈를 보이는 Body Mesh에 전달한다.
	// Body Mesh는 별도로 포즈를 계산하지 않고 Leader Pose를 따라간다.
	VisibleBodyMesh->SetLeaderPoseComponent(DriverMesh, true, false);
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	// 달리는 동안 스태미나를 소비하고, 휴식 중에는 지연 후 회복한다.
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
	// 공격 중에는 이동 입력을 받지 않는다.
	if (CombatComponent->IsAttacking())
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
	if (CombatComponent->IsAttacking())
	{
		return;
	}

	// 스태미나가 없으면 달리기를 시작하지 않는다.
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
	CombatComponent->RequestAttack();
}

void APlayerCharacter::StartJump()
{
	if (CombatComponent->IsAttacking())
	{
		return;
	}
	Jump();
}
