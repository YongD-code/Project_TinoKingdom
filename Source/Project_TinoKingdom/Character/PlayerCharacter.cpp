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

	//?§ÌÉØ Ïª¥Ìè¨?åÌä∏ ?ùÏÑ±??
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));

	// ?åÎ†à?¥Ïñ¥???ÄÏßÅÏûÑ Í¥Ä??Í∞íÎì§
	MovementComponent->MaxAcceleration = 500.f;
	MovementComponent->BrakingDecelerationWalking = 450.f;
	MovementComponent->GroundFriction = 6.f;

	MovementComponent->bUseSeparateBrakingFriction = true;
	MovementComponent->BrakingFriction = 0.4f;
	MovementComponent->BrakingFrictionFactor = 1.f;

	// Ï∂©Îèå ?ÑÎ¶¨???ÅÏö©
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

	// true - bForceUpdate Í∞íÏù∏??true, false ?????ÅÍ??ÜÏùÑ?? ?¥Ï∞®??BeginPlay()?êÏÑú ?§Ìñâ?òÎãàÍπ?
	// fasle - CharacterMesh0???¨Ï¶àÎ•??∞ÎùºÍ∞Ä??BodyÍ∞Ä Î≥ÑÎèÑÎ°?TickPoseÎ•??§Ìñâ?òÏ? ?äÍ≤å
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

	// Í≥µÍ≤©???¥Î? ?πÏÖò???ïÌï¥?ìÏ? ?Ä?ÅÎßå???ÄÍ≤©ÏùÑ ?àÎã§Î©??¥ÌõÑ Tick?êÏÑú Sweep???ÑÏöî ?ÜÏùå
	// AttackSection.MaxHitTargets > 0 Ï°∞Í±¥?Ä MaxHitTargets == 0?¥Î©¥ ?ÄÍ≤????úÌïú???ÜÍ∏∞ ?åÎ¨∏
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

		// ?ÑÏû¨ Sweep?êÏÑú ÏµúÎ? ?Ä???òÏóê ?ÑÎã¨?àÎäîÏßÄ ?ïÏù∏
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


	//?±Îßà???§ÌÖåÎØ∏ÎÇò Í∞êÏÜå Í≥ÑÏÇ∞ Ï≤¥ÌÅ¨
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

	// Anim Notify State Íµ¨Í∞Ñ???àÎ¨¥ ÏßßÏúºÎ©?Í≤Ä?¨Í? ???†ÍπåÎ¥?Ï¶âÏãú ??Î≤?Í≤Ä??
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
	// Í≥µÍ≤© Ï§ëÏóê???¥Îèô ?ÖÎ†• ?°ÏÖò??Î∂àÍ??•ÌïòÍ≤?
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

	//?§ÌÖåÎØ∏ÎÇò ?¨Ïö©?¥ÏÑú ?¨Î¶¨Í∏??ÑÏãú Ï∂îÍ?Î≥?
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

	// ?¨ÏÉù ?±Í≥µ ??
	QueuedComboIndex = 0;
	bComboInputWindowOpen = false;
	bComboInputConsumed = false;

	// Data Asset??Ï≤?Î≤àÏß∏ ?πÏÖò?ºÎ°ú ?¥Îèô
	AnimInstance->Montage_JumpToSection(FirstSectionName, AttackData->AttackMontage);
	FOnMontageEnded MontageEndedDelegate;

	// Î™ΩÌ?ÏßÄ Ï¢ÖÎ£å ?∏Î¶¨Í≤åÏù¥???±Î°ù
	MontageEndedDelegate.BindUObject(this, &APlayerCharacter::OnAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AttackData->AttackMontage);

	// Í∏∞Ï°¥ ?¥Îèô ?çÎèÑ ?úÍ±∞
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
}

void APlayerCharacter::TryQueueNextCombo(UAnimInstance* AnimInstance, const UAttackComboData* AttackData)
{
	// ?ÖÎ†• ?àÏö© Íµ¨Í∞Ñ???ÑÎãàÍ±∞ÎÇò ?¥Î? ?ÖÎ†•??Î∞õÏïò?§Î©¥ Î¨¥Ïãú
	if (!bComboInputWindowOpen || bComboInputConsumed)
	{
		return;
	}

	const int32 NextComboIndex = QueuedComboIndex + 1;

	// ÎßàÏ?Îß?ÏΩ§Î≥¥?ºÏÑú ?§Ïùå Í≥µÍ≤©???ÜÎã§Î©?Î¨¥Ïãú
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

	// ÎßåÏïΩ???åÎ†à?¥Ïñ¥Í∞Ä Î™ΩÌ?ÏßÄÎ•??åÎ†à?¥Ìïò???ÑÏ§ë ?äÍ≤º???åÎ? ?ÄÎπ?
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
	// ?¥Ï†ú??Î™ΩÌ?ÏßÄ ÏßÅÏ†ë Í≤Ä?¨Ìï† ?ÑÏöî X
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
