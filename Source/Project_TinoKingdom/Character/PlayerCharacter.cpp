// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Project_TinoKingdom/Constants/TinoCollision.h"
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

	// 스탯 컴포넌트를 기본 서브오브젝트로 생성한다.
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));

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

void APlayerCharacter::PerformAttackTrace()
{
	if (!bAttackHitWindowOpen)
	{
		return;
	}

	const UAttackComboData* AttackData = GetCurrentAttackData();
	const FComboAttackSectionData& AttackSection = AttackData->ComboSection[ActiveAttackSectionIndex];

	// 이미 최대 타격 대상 수에 도달했다면 이후 Tick의 Sweep을 생략한다.
	// MaxHitTargets가 0이면 타격 대상 수에 제한을 두지 않는다.
	if (AttackSection.MaxHitTargets > 0 && HitActorsThisWindow.Num() >= AttackSection.MaxHitTargets)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const FVector Forward = GetActorForwardVector();
	const FVector Up = GetActorUpVector();

	const FVector Start = GetActorLocation() + Forward * AttackSection.TraceStartForwardOffset
						+ Up * AttackSection.TraceHeightOffset;
	const FVector End = Start + Forward * AttackSection.TraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TinoMeleeTrace), false, this);
	TArray<FHitResult> HitResults;
	World->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, TinoCollision::Action,
		FCollisionShape::MakeSphere(AttackSection.TraceRadius), QueryParams);

	HitResults.Sort(
		[](const FHitResult& A, const FHitResult& B)
		{
			return A.Distance < B.Distance;
		});

	const FColor DebugColor = HitResults.IsEmpty() ? FColor::Green : FColor::Red;
	DrawDebugSphere(World, Start, AttackSection.TraceRadius, 16, DebugColor, false, 0.1f, 0, 1.f);
	DrawDebugSphere(World, End, AttackSection.TraceRadius, 16, DebugColor, false, 0.1f, 0, 1.f);
	DrawDebugLine(World, Start, End, DebugColor, false, 0.1f, 0, 1.f);

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!IsValid(HitActor))
		{
			continue;
		}

		const TWeakObjectPtr<AActor> HitActorKey(HitActor);
		if (HitActorsThisWindow.Contains(HitActorKey))
		{
			continue;
		}
		HitActorsThisWindow.Add(HitActorKey);

		UGameplayStatics::ApplyPointDamage(HitActor, AttackSection.Damage, Forward, HitResult,
			GetController(), this, UDamageType::StaticClass());

        UE_LOG(LogTinoCombat, Log, TEXT("%s attack hit %s"), *GetNameSafe(this), *GetNameSafe(HitActor));

		// 현재 Sweep에서 최대 타격 대상 수에 도달하면 반복을 종료한다.
		if (AttackSection.MaxHitTargets > 0 && HitActorsThisWindow.Num() >= AttackSection.MaxHitTargets)
		{
			break;
		}
	}
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

void APlayerCharacter::SetComboInputWindowOpen(bool bIsOpen)
{
	bComboInputWindowOpen = bIsOpen;
	if (bIsOpen)
	{
		bComboInputConsumed = false;
	}
}

void APlayerCharacter::BeginAttackHitWindow()
{
	bAttackHitWindowOpen = false;
	ActiveAttackSectionIndex = INDEX_NONE;
	HitActorsThisWindow.Reset();

	const int32 FoundSectionIndex = FindActiveComboAttackSectionIndex();
	const UAttackComboData* AttackData = GetCurrentAttackData();
	if (!IsValid(AttackData) || !AttackData->ComboSection.IsValidIndex(FoundSectionIndex))
	{
        UE_LOG(LogTinoCombat, Error, TEXT("%s: active attack section data was not found."), *GetNameSafe(this));
		return;
	}

	ActiveAttackSectionIndex = FoundSectionIndex;
	bAttackHitWindowOpen = true;

	// Notify 구간이 매우 짧아도 판정이 누락되지 않도록 즉시 한 번 검사한다.
	PerformAttackTrace();
}

void APlayerCharacter::TickAttackHitWindow(float DeltaTime)
{
	PerformAttackTrace();
}

void APlayerCharacter::EndAttackHitWindow()
{
	bAttackHitWindowOpen = false;
	ActiveAttackSectionIndex = INDEX_NONE;
	HitActorsThisWindow.Reset();
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
        UE_LOG(LogTinoCombat, Error, TEXT("%s: AnimInstance is invalid."), *GetNameSafe(this));
		return;
	}

	if (!IsValid(AttackData))
	{
        UE_LOG(LogTinoCombat, Error, TEXT("%s: AttackData is invalid."), *GetNameSafe(this));
		return;
	}

	if (!IsValid(AttackData->AttackMontage))
	{
        UE_LOG(LogTinoCombat, Error, TEXT("%s: AttackMontage is invalid."), *GetNameSafe(this));
		return;
	}

	if (!AttackData->ComboSection.IsValidIndex(0))
	{
        UE_LOG(LogTinoCombat, Error, TEXT("%s: ComboSection is empty."), *GetNameSafe(this));
		return;
	}

	const FName FirstSectionName = AttackData->ComboSection[0].SectionName;
	if (!AttackData->AttackMontage->IsValidSectionName(FirstSectionName))
	{
        UE_LOG(LogTinoCombat, Error, TEXT("%s: montage %s does not contain section %s."),
			*GetNameSafe(this), *GetNameSafe(AttackData->AttackMontage), *FirstSectionName.ToString());
		return;
	}

	const float MontageDuration = AnimInstance->Montage_Play(AttackData->AttackMontage);
	if (MontageDuration <= 0.f)
	{
        UE_LOG(LogTinoCombat, Error, TEXT("%s: failed to play attack montage %s."),
			*GetNameSafe(this), *GetNameSafe(AttackData->AttackMontage));
		ResetCombo();
		return;
	}

	// 몽타주 재생 성공 후 첫 번째 콤보 상태로 초기화한다.
	QueuedComboIndex = 0;
	bComboInputWindowOpen = false;
	bComboInputConsumed = false;

	// 데이터 애셋에 정의된 첫 번째 콤보 섹션으로 이동한다.
	AnimInstance->Montage_JumpToSection(FirstSectionName, AttackData->AttackMontage);
	FOnMontageEnded MontageEndedDelegate;

	// 공격 몽타주 종료 시 콤보 상태를 초기화할 델리게이트를 등록한다.
	MontageEndedDelegate.BindUObject(this, &APlayerCharacter::OnAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AttackData->AttackMontage);

	// 공격 시작 시 남아 있는 이동 입력과 속도를 제거한다.
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
}

void APlayerCharacter::TryQueueNextCombo(UAnimInstance* AnimInstance, const UAttackComboData* AttackData)
{
	// 콤보 입력 허용 구간이 아니거나 이미 입력을 소비했다면 무시한다.
	if (!bComboInputWindowOpen || bComboInputConsumed)
	{
		return;
	}

	const int32 NextComboIndex = QueuedComboIndex + 1;

	// 다음 콤보 섹션이 없으면 추가 입력을 무시한다.
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

	// 몽타주가 중단된 경우에도 남아 있는 공격 판정 상태를 정리한다.
	EndAttackHitWindow();
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
	// 콤보 인덱스로 공격 상태를 관리하므로 몽타주를 직접 검사하지 않는다.
	return QueuedComboIndex != INDEX_NONE;
}

int32 APlayerCharacter::FindActiveComboAttackSectionIndex() const
{
	const UAttackComboData* AttackData = GetCurrentAttackData();
	const USkeletalMeshComponent* CharacterMesh = GetMesh();

	if (!IsValid(AttackData) || !IsValid(AttackData->AttackMontage) || !IsValid(CharacterMesh))
	{
		return INDEX_NONE;
	}

	const UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return INDEX_NONE;
	}

	const FName CurrentSectionName = AnimInstance->Montage_GetCurrentSection(AttackData->AttackMontage);
	if (CurrentSectionName.IsNone())
	{
		return INDEX_NONE;
	}

	return AttackData->ComboSection.IndexOfByPredicate(
		[&CurrentSectionName](const FComboAttackSectionData& SectionData)
		{
			return SectionData.SectionName == CurrentSectionName;
		});
}
