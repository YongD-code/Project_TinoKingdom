// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayEffect.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Math/RotationMatrix.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Project_TinoKingdom/Component/ReactionComponent.h"
#include "Project_TinoKingdom/Component/CookingComponent.h"
#include "Project_TinoKingdom/Component/CookingRecipeBookComponent.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/Component/TinoCombatComponent.h"
#include "Project_TinoKingdom/Component/TinoEquipmentComponent.h"
#include "Project_TinoKingdom/DataAsset/EquipmentLoadoutData.h"
#include "Project_TinoKingdom/Component/TinoStateComponent.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"
#include "Project_TinoKingdom/Component/DodgeComponent.h"
#include "Project_TinoKingdom/Component/DialogueComponent.h"
#include "Project_TinoKingdom/Component/QuestComponent.h"
#include "TinoNPCCharacter.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "Project_TinoKingdom/Component/TargetingComponent.h"
#include "Project_TinoKingdom/Component/PlayerProgressionComponent.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAbilitySystemComponent.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAttributeSet.h"
#include "Project_TinoKingdom/GameMode/TinoGameInstance.h"
#include "Project_TinoKingdom/Player/TinoPlayerController.h"
#include "Project_TinoKingdom/Interface/TargetableInterface.h"
#include "Project_TinoKingdom/World/SecretPlaceEntrance.h"

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
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;

	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;
	CameraBoom->CameraLagMaxDistance = 40.f;
	CameraBoom->bUseCameraLagSubstepping = true;
	CameraBoom->bEnableCameraRotationLag = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->SetRelativeLocation(FVector(15.082244f, 0.f, 195.552685f));
	FollowCamera->SetRelativeRotation(FRotator(-23.492965f, 0.f, 0.f));
	FollowCamera->SetFieldOfView(90.f);
	FollowCamera->bUsePawnControlRotation = false;

	// 스탯 컴포넌트를 기본 서브오브젝트로 생성한다.
	AbilitySystemComponent = CreateDefaultSubobject<UTinoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UTinoAttributeSet>(TEXT("AttributeSet"));
	ProgressionComponent = CreateDefaultSubobject<UPlayerProgressionComponent>(TEXT("ProgressionComponent"));
	CombatComponent = CreateDefaultSubobject<UTinoCombatComponent>(TEXT("CombatComponent"));
	EquipmentComponent = CreateDefaultSubobject<UTinoEquipmentComponent>(TEXT("EquipmentComponent"));
	ReactionComponent = CreateDefaultSubobject<UReactionComponent>(TEXT("ReactionComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	CookingComponent = CreateDefaultSubobject<UCookingComponent>(TEXT("CookingComponent"));
	CookingRecipeBookComponent = CreateDefaultSubobject<UCookingRecipeBookComponent>(TEXT("CookingRecipeBookComponent"));
	CharacterStateComponent = CreateDefaultSubobject<UTinoStateComponent>(TEXT("CharacterStateComponent"));
	DodgeComponent = CreateDefaultSubobject<UDodgeComponent>(TEXT("DodgeComponent"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
	DialogueComponent = CreateDefaultSubobject<UDialogueComponent>(TEXT("DialogueComponent"));
	QuestComponent = CreateDefaultSubobject<UQuestComponent>(TEXT("QuestComponent"));

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

UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitialSpawnTransform = GetActorTransform();
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	const UTinoAttributeSet* RegisteredAttributeSet = AbilitySystemComponent->GetSet<UTinoAttributeSet>();

	const bool bAttributeSetRegistered = ensureMsgf(
		RegisteredAttributeSet == AttributeSet,
		TEXT("TinoAttributeSet이 ASC에 올바르게 등록되지 않았습니다.")
	);
	
	if (bAttributeSetRegistered && InitializeDefaultAttributes())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"GAS 초기화 완료: "
				"Health %.1f / %.1f, "
				"Stamina %.1f / %.1f, "
				"Attack %.1f, Defense %.1f"
			),
			AttributeSet->GetHealth(),
			AttributeSet->GetMaxHealth(),
			AttributeSet->GetStamina(),
			AttributeSet->GetMaxStamina(),
			AttributeSet->GetAttackPower(),
			AttributeSet->GetDefense()
		);
	}

	// OpenLevel 직전에 GameInstance에 저장한 상태가 있으면 기본 능력치 초기화 다음에 덮어쓴다.
	if (UTinoGameInstance* TinoGameInstance = Cast<UTinoGameInstance>(GetGameInstance()))
	{
		TinoGameInstance->RestorePlayerState(this);
	}
	
	DefaultCameraArmLength = CameraBoom->TargetArmLength;
	DefaultCameraSocketOffset = CameraBoom->SocketOffset;
	DefaultCameraTargetOffset = CameraBoom->TargetOffset;
	DefaultFollowCameraRelativeLocation = FollowCamera->GetRelativeLocation();
	DefaultFollowCameraRelativeRotation = FollowCamera->GetRelativeRotation();
	DefaultCameraFieldOfView = FollowCamera->FieldOfView;
	DefaultCameraLagSpeed = CameraBoom->CameraLagSpeed;
	DefaultCameraLagMaxDistance = CameraBoom->CameraLagMaxDistance;

	TargetingComponent->OnTargetChanged.AddUniqueDynamic(this, &APlayerCharacter::HandleLockOnTargetChanged);
	EquipmentComponent->OnEquipmentChanged.AddUniqueDynamic(this, &APlayerCharacter::HandleEquipmentChanged);
	// EquipmentComponent의 BeginPlay가 먼저 실행됐을 수 있기 때문에 현재 값도 직접 반영
	if (UEquipmentLoadoutData* CurrentLoadout = EquipmentComponent->GetCurrentLoadout())
	{
		HandleEquipmentChanged(CurrentLoadout);
	}
	
	UpdateRotationMode();

	static const FName AnimationBodyTag(TEXT("AnimationBody"));
	VisibleBodyMesh = FindComponentByTag<USkeletalMeshComponent>(AnimationBodyTag);

	USkeletalMeshComponent* DriverMesh = GetMesh();
	DriverMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	DriverMesh->SetHiddenInGame(true);

	// 숨겨진 Driver Mesh의 포즈를 보이는 Body Mesh에 전달한다.
	// Body Mesh는 별도로 포즈를 계산하지 않고 Leader Pose를 따라간다.
	VisibleBodyMesh->SetLeaderPoseComponent(DriverMesh, true, false);

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (IsValid(PlayerController->PlayerCameraManager) && StartupFadeInDuration > 0.f)
		{
			PlayerController->PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
			PlayerController->PlayerCameraManager->StartCameraFade(1.f, 0.f, StartupFadeInDuration, FLinearColor::Black, false, false);
		}
	}
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	ClearRespawnSequence();
	
	// 장비창이 열린 채 사망해도 전역 시간을 원래대로 복구
	StopSlowMotion();
	StopAiming();

	TargetingComponent->ClearTarget();
	TargetingComponent->OnTargetChanged.RemoveDynamic(this, &APlayerCharacter::HandleLockOnTargetChanged);
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &APlayerCharacter::HandleEquipmentChanged);
	}
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetingComponent->IsLockedOn())
	{
		UpdateLockOnCamera(DeltaTime);
	}
	if (bCameraTransition)
	{
		UpdateCameraTransition(DeltaTime);
	}
	
	if (bDeathHandled || AttributeSet == nullptr)
	{
		return;
	}

	if (bRunning)
	{
		const float CurrentStamina = AttributeSet->GetStamina();
		const float StaminaCost = RunningStamina * DeltaTime;
		const float NewStamina = FMath::Max(CurrentStamina - StaminaCost, 0.0f);
		
		if (!FMath::IsNearlyEqual(CurrentStamina, NewStamina))
		{
			AttributeSet->SetStamina(NewStamina);
		}
		if (NewStamina <= 0.f)
		{
			StopRunning();
		}
		return;
	}

	if (StaminaDelayTime > 0.0f)
	{
		StaminaDelayTime = FMath::Max(StaminaDelayTime - DeltaTime, 0.f);
		return;
	}

	const float CurrentStamina = AttributeSet->GetStamina();
	const float MaxStamina = AttributeSet->GetMaxStamina();
	if (CurrentStamina >= MaxStamina)
	{
		return;
	}
	
	const float NewStamina = FMath::Min(CurrentStamina + RecoverStaminaWhileRest * DeltaTime, MaxStamina);
	if (NewStamina > CurrentStamina)
	{
		AttributeSet->SetStamina(NewStamina);
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
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerCharacter::MoveDebugFlyUp);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Attack);

	EnhancedInputComponent->BindAction(ToggleCharacterMenuAction,ETriggerEvent::Started,this,&APlayerCharacter::ToggleCharacterMenu);

	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::StartRunning);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRunning);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &APlayerCharacter::StopRunning);

	EnhancedInputComponent->BindAction(ToggleEquipmentMenuAction, ETriggerEvent::Started, this, &APlayerCharacter::OpenEquipmentWheel);
	EnhancedInputComponent->BindAction(ToggleEquipmentMenuAction, ETriggerEvent::Completed, this, &APlayerCharacter::ConfirmEquipmentWheel);
	EnhancedInputComponent->BindAction(ToggleEquipmentMenuAction, ETriggerEvent::Canceled, this, &APlayerCharacter::CancelEquipmentWheel);

	EnhancedInputComponent->BindAction(ToggleDebugAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleDebugFly);

	EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &APlayerCharacter::Dodge);

	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &APlayerCharacter::StartAiming);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopAiming);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &APlayerCharacter::StopAiming);

	EnhancedInputComponent->BindAction(TargetingAction, ETriggerEvent::Started, this, &APlayerCharacter::RequestTargeting);

	// 새 Input Action이 아직 지정되지 않은 블루프린트도 안전하게 실행될 수 있게 선택적으로 바인딩한다.
	if (OpenSecretPlaceAction != nullptr)
	{
		EnhancedInputComponent->BindAction(
			OpenSecretPlaceAction,
			ETriggerEvent::Started,
			this,
			&APlayerCharacter::OpenSecretPlace);
	}

	// 대화 시작은 기본 컨텍스트에, 진행과 취소는 대화 컨텍스트에 매핑되어 있다.
	EnhancedInputComponent->BindAction(DialInteractAction, ETriggerEvent::Started, this, &APlayerCharacter::Interact);
	EnhancedInputComponent->BindAction(DialAdvanceAction, ETriggerEvent::Started, this, &APlayerCharacter::DialogueAdvancePressed);
	EnhancedInputComponent->BindAction(DialAdvanceAction, ETriggerEvent::Completed, this, &APlayerCharacter::DialogueAdvanceReleased);
	EnhancedInputComponent->BindAction(DialAdvanceAction, ETriggerEvent::Canceled, this, &APlayerCharacter::DialogueAdvanceReleased);
}

void APlayerCharacter::OpenSecretPlace()
{
	TryOpenSecretPlace();
}

bool APlayerCharacter::TryOpenSecretPlace()
{
	if (bLevelTravelInProgress || bDeathHandled || SecretPlaceLevelName.IsNone())
	{
		return false;
	}

	if (IsValid(DialogueComponent) && DialogueComponent->IsInDialogue())
	{
		return false;
	}

	UTinoGameInstance* TinoGameInstance = Cast<UTinoGameInstance>(GetGameInstance());
	if (!ensureMsgf(TinoGameInstance != nullptr,
		TEXT("Project Settings의 GameInstance Class가 TinoGameInstance로 지정되지 않았습니다.")))
	{
		return false;
	}

	if (!TinoGameInstance->CapturePlayerState(this))
	{
		return false;
	}

	bLevelTravelInProgress = true;
	UGameplayStatics::OpenLevel(this, SecretPlaceLevelName);
	return true;
}

bool APlayerCharacter::TryUseUsableItem(const FName ItemId)
{
	if (ItemId.IsNone() || bDeathHandled || bLevelTravelInProgress)
	{
		return false;
	}

	ASecretPlaceEntrance* SecretPlaceEntrance = Cast<ASecretPlaceEntrance>(
		UGameplayStatics::GetActorOfClass(this, ASecretPlaceEntrance::StaticClass()));
	if (!IsValid(SecretPlaceEntrance))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Usable 아이템 %s 사용 실패: 현재 월드에 SecretPlaceEntrance가 없습니다."),
			*ItemId.ToString());
		return false;
	}

	return SecretPlaceEntrance->TryUseItem(this, ItemId);
}

void APlayerCharacter::Interact()
{
	if (!IsValid(DialogueComponent) || DialogueComponent->IsInDialogue())
	{
		return;
	}

	if (ATinoNPCCharacter* NearbyNPC = FindNearbyNPC())
	{
		if (!DialogueComponent->StartDialogue(NearbyNPC))
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC 대화 시작 실패: %s"), *GetNameSafe(NearbyNPC));
		}
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("상호작용 범위 안에서 NPC를 찾지 못했습니다."));
}

void APlayerCharacter::DialogueAdvancePressed()
{
	if (IsValid(DialogueComponent))
	{
		DialogueComponent->OnAdvancePressed();
	}
}

void APlayerCharacter::DialogueAdvanceReleased()
{
	if (IsValid(DialogueComponent))
	{
		DialogueComponent->OnAdvanceReleased();
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent == nullptr || Controller == nullptr)
	{
		return;
	}

	const bool bDebugFlying = MovementComponent->IsFlying();

	// 디버그 비행 중에는 사망/공격/피격 상태와 관계없이 이동할 수 있게 한다.
	if (!bDebugFlying && !CharacterStateComponent->CanPerformAction(ETinoAction::Move))
	{
		return;
	}

	const FVector2D MovementInput = Value.Get<FVector2D>();
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	const FRotator ForwardRotation = bDebugFlying ? ControlRotation : YawRotation;

	// 비행 중에는 카메라 Pitch를 포함하므로 위/아래를 바라보고 전진하면 상승/하강한다.
	const FVector Forward = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MovementInput.Y);
	AddMovementInput(Right, MovementInput.X);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	if (TargetingComponent->IsLockedOn())
	{
		return;
	}
	const FVector2D LookInput = Value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void APlayerCharacter::ToggleCharacterMenu()
{
	ATinoPlayerController* PlayerController = Cast<ATinoPlayerController>(GetController());
	PlayerController->ToggleCharacterMenu();
}

void APlayerCharacter::ToggleCookingMenu()
{
	ATinoPlayerController* PlayerController = Cast<ATinoPlayerController>(GetController());
	if (PlayerController == nullptr)
	{
		return;
	}

	PlayerController->ToggleCookingMenu(CookingComponent, InventoryComponent);
}

bool APlayerCharacter::IsNearCookingPot() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const FVector PlayerLocation = GetActorLocation();
	const float InteractionRadiusSquared = FMath::Square(CookingPotInteractionRadius);

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		const AActor* Actor = *ActorIterator;
		if (!IsValid(Actor) || Actor == this)
		{
			continue;
		}

		const bool bTaggedCookingPot =
			!CookingPotActorTag.IsNone() && Actor->ActorHasTag(CookingPotActorTag);
		const bool bNamedCookingPot =
			!CookingPotClassNameKeyword.IsEmpty() &&
			(Actor->GetClass()->GetName().Contains(CookingPotClassNameKeyword) ||
			 Actor->GetName().Contains(CookingPotClassNameKeyword));

		if (!bTaggedCookingPot && !bNamedCookingPot)
		{
			continue;
		}

		if (FVector::DistSquared(PlayerLocation, Actor->GetActorLocation()) <= InteractionRadiusSquared)
		{
			return true;
		}
	}

	return false;
}

void APlayerCharacter::StartRunning()
{
	if (ShouldUseStrafeMovement())
	{
		return;
	}
	if (!CharacterStateComponent->CanPerformAction(ETinoAction::Sprint))
	{
		return;
	}

	// 스태미나가 없으면 달리기를 시작하지 않는다.
	if (AttributeSet->GetStamina() <= 0.0f)
	{
		return;
	}

	bRunning = true;
	UpdateMovementSpeed();
}

void APlayerCharacter::StopRunning()
{
	if (bRunning)
	{
		StaminaDelayTime = StaminaDelay;
	}
	bRunning = false;
	UpdateMovementSpeed();
}

ATinoNPCCharacter* APlayerCharacter::FindNearbyNPC() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const FVector Center = GetActorLocation();

	TArray<FOverlapResult> Overlaps;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NPCInterractionCheck), false, this);
	Params.AddIgnoredActor(this);

	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(InteractionRadius),
		Params
	);

	if (bDrawInteractionDebug)
	{
		DrawDebugSphere(World, Center, InteractionRadius, 24, FColor::Green, false, 1.0f);
	}

	// OverlapMulti는 결과를 거리순으로 정렬해 주지 않으므로 가장 가까운 NPC를 직접 고른다.
	// 제곱 거리로 비교해 매 비교마다 제곱근을 계산하지 않는다.
	ATinoNPCCharacter* ClosestNPC = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	if (bHit)
	{
		for (const FOverlapResult& Result : Overlaps)
		{
			ATinoNPCCharacter* NPC = Cast<ATinoNPCCharacter>(Result.GetActor());

			if (NPC == nullptr)
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared(Center, NPC->GetActorLocation());

			if (DistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = DistanceSquared;
				ClosestNPC = NPC;
			}
		}
	}

	if (ClosestNPC != nullptr)
	{
		return ClosestNPC;
	}

	for (TActorIterator<ATinoNPCCharacter> NPCIterator(World); NPCIterator; ++NPCIterator)
	{
		ATinoNPCCharacter* NPC = *NPCIterator;
		if (!IsValid(NPC))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Center, NPC->GetActorLocation());
		if (DistanceSquared > FMath::Square(InteractionRadius))
		{
			continue;
		}

		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestNPC = NPC;
		}
	}

	return ClosestNPC;
}

void APlayerCharacter::Attack()
{
	if (!CharacterStateComponent->CanPerformAction(ETinoAction::Attack))
	{
		return;
	}
	CombatComponent->RequestAttack();
}

void APlayerCharacter::StartJump()
{
	// 비행 중 Space는 점프가 아니라 MoveDebugFlyUp에서 상승 입력으로 처리한다.
	if (GetCharacterMovement()->IsFlying())
	{
		return;
	}

	if (!CharacterStateComponent->CanPerformAction(ETinoAction::Jump))
	{
		return;
	}
	if (!CanJump())
	{
		return;
	}
	
	const float StaminaBeforeJump = AttributeSet->GetStamina();
	if (StaminaBeforeJump < JumpStamina)
	{
		return;
	}
	Jump();
	
	const float NewStamina = FMath::Max(StaminaBeforeJump - JumpStamina, 0.f);
	AttributeSet->SetStamina(NewStamina);
	StaminaDelayTime = StaminaDelay;
}

void APlayerCharacter::MoveDebugFlyUp()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr || !Movement->IsFlying())
	{
		return;
	}

	AddMovementInput(FVector::UpVector, 1.f);
}

void APlayerCharacter::ToggleDebugFly()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr)
	{
		return;
	}

	Movement->StopMovementImmediately();

	if (Movement->IsFlying())
	{
		Movement->bCheatFlying = false;
		Movement->SetMovementMode(MOVE_Falling);
		return;
	}

	StopRunning();
	Movement->bCheatFlying = true;
	Movement->MaxFlySpeed = 6000.f;
	Movement->MaxAcceleration = 6000.f;
	Movement->BrakingDecelerationFlying = 6000.f;
	Movement->SetMovementMode(MOVE_Flying);
}

void APlayerCharacter::Dodge()
{
	if (!CharacterStateComponent->CanPerformAction(ETinoAction::Dodge))
	{
		return;
	}
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent->IsMovingOnGround())
	{
		return;
	}

	const float StaminaBeforeDodge = AttributeSet->GetStamina();
	if (StaminaBeforeDodge < DodgeStamina)
	{
		return;
	}
	
	FVector DodgeDirection = GetPendingMovementInputVector();
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetLastMovementInputVector();
	}
	const bool bUseStrafeDodge = ShouldUseStrafeMovement();

	if (!DodgeComponent->StartDodge(DodgeDirection, bUseStrafeDodge))
	{
		return;
	}
	
	StopRunning();
	
	const float NewStamina = FMath::Max(StaminaBeforeDodge - DodgeStamina, 0.f);
	AttributeSet->SetStamina(NewStamina);
	StaminaDelayTime = StaminaDelay;
}

void APlayerCharacter::StartAiming()
{
	if (CharacterStateComponent->HasStateTag(TinoGameplayTags::State_Dead))
	{
		return;
	}
	bIsAiming = true;
	UpdateRotationMode();
	bCameraTransition = true;

	if (ATinoPlayerController* PlayerController = Cast<ATinoPlayerController>(GetController()))
	{
		PlayerController->SetCrosshairVisible(true);
	}
}

void APlayerCharacter::StopAiming()
{
	bIsAiming = false;
	UpdateRotationMode();
	bCameraTransition = true;
	if (ATinoPlayerController* PlayerController = Cast<ATinoPlayerController>(GetController()))
	{
		PlayerController->SetCrosshairVisible(false);
	}
}

void APlayerCharacter::RequestTargeting()
{
	if (TargetingComponent->IsLockedOn())
	{
		TargetingComponent->ClearTarget();
		return;
	}
	if (!bIsAiming)
    {
        UE_LOG(LogTemp, Warning, TEXT("Lock On 무시: Aim 모드가 아닙니다."));
        return;
    }

    TargetingComponent->TryLockOnFromCrosshair();
}

void APlayerCharacter::UpdateCameraTransition(float DeltaTime)
{
	float DesiredArmLength = DefaultCameraArmLength;
	FVector DesiredSocketOffset = DefaultCameraSocketOffset;
	FVector DesiredTargetOffset = DefaultCameraTargetOffset;
	FVector DesiredFollowCameraLocation = DefaultFollowCameraRelativeLocation;
	FRotator DesiredFollowCameraRotation = DefaultFollowCameraRelativeRotation;
	float DesiredFieldOfView = DefaultCameraFieldOfView;

	if (TargetingComponent->IsLockedOn())
	{
		DesiredArmLength = LockOnCameraArmLength;
		DesiredSocketOffset = LockOnCameraSocketOffset;
		DesiredTargetOffset = LockOnCameraTargetOffset;
		DesiredFollowCameraLocation = LockOnFollowCameraRelativeLocation;
		DesiredFollowCameraRotation = LockOnFollowCameraRelativeRotation;
		DesiredFieldOfView = LockOnFieldOfView;
	}
	else if (bIsAiming)
	{
		DesiredArmLength = AimCameraArmLength;
		DesiredSocketOffset = AimCameraSocketOffset;
		DesiredFieldOfView = AimFieldOfView;
	}

	const float NewArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength, DesiredArmLength, DeltaTime, AimCameraInterpSpeed);
	const FVector NewSocketOffset = FMath::VInterpTo(
		CameraBoom->SocketOffset, DesiredSocketOffset, DeltaTime, AimCameraInterpSpeed);
	const FVector NewTargetOffset = FMath::VInterpTo(
		CameraBoom->TargetOffset, DesiredTargetOffset, DeltaTime, AimCameraInterpSpeed);
	const FVector NewFollowCameraLocation = FMath::VInterpTo(
		FollowCamera->GetRelativeLocation(), DesiredFollowCameraLocation, DeltaTime, AimCameraInterpSpeed);
	const FRotator NewFollowCameraRotation = FMath::RInterpTo(
		FollowCamera->GetRelativeRotation(), DesiredFollowCameraRotation, DeltaTime, AimCameraInterpSpeed);
	const float NewFieldOfView = FMath::FInterpTo(
		FollowCamera->FieldOfView, DesiredFieldOfView, DeltaTime, AimCameraInterpSpeed);

	CameraBoom->TargetArmLength = NewArmLength;
	CameraBoom->SocketOffset = NewSocketOffset;
	CameraBoom->TargetOffset = NewTargetOffset;
	FollowCamera->SetRelativeLocationAndRotation(NewFollowCameraLocation, NewFollowCameraRotation);
	FollowCamera->SetFieldOfView(NewFieldOfView);

	const bool bReachedTarget =
		FMath::IsNearlyEqual(NewArmLength, DesiredArmLength, 0.1f) &&
		NewSocketOffset.Equals(DesiredSocketOffset, 0.1f) &&
		NewTargetOffset.Equals(DesiredTargetOffset, 0.1f) &&
		NewFollowCameraLocation.Equals(DesiredFollowCameraLocation, 0.1f) &&
		NewFollowCameraRotation.Equals(DesiredFollowCameraRotation, 0.01f) &&
		FMath::IsNearlyEqual(NewFieldOfView, DesiredFieldOfView, 0.01f);

	if (!bReachedTarget)
	{
		return;
	}

	CameraBoom->TargetArmLength = DesiredArmLength;
	CameraBoom->SocketOffset = DesiredSocketOffset;
	CameraBoom->TargetOffset = DesiredTargetOffset;
	FollowCamera->SetRelativeLocationAndRotation(DesiredFollowCameraLocation, DesiredFollowCameraRotation);
	FollowCamera->SetFieldOfView(DesiredFieldOfView);
	bCameraTransition = false;
}

bool APlayerCharacter::ShouldUseStrafeMovement() const
{
	return bIsAiming || TargetingComponent->IsLockedOn();
}

void APlayerCharacter::UpdateRotationMode()
{
	const bool bUseStrafeMovement = ShouldUseStrafeMovement();
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	bUseControllerRotationYaw = bUseStrafeMovement;
	MovementComponent->bOrientRotationToMovement = !bUseStrafeMovement;
	MovementComponent->bUseControllerDesiredRotation = false;

	if (bUseStrafeMovement)
	{
		StopRunning();
	}
	else
	{
		UpdateMovementSpeed();
	}
}

void APlayerCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	if (ShouldUseStrafeMovement())
	{
		MovementComponent->MaxWalkSpeed = StrafeSpeed;
		return;
	}

	MovementComponent->MaxWalkSpeed = bRunning ? RunSpeed : WalkSpeed;
}

void APlayerCharacter::UpdateLockOnCamera(float DeltaTime)
{
	AActor* CurrentTarget = TargetingComponent->GetCurrentTarget();
	const FVector TargetLocation = ITargetableInterface::Execute_GetLockOnLocation(CurrentTarget);
	const FVector CameraLocation = FollowCamera->GetComponentLocation();

	FRotator TargetRotation = (TargetLocation - CameraLocation).Rotation();
	TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, LockOnCameraMinPitch, LockOnCameraMaxPitch);
	TargetRotation.Roll = 0.f;

	const FRotator NewControllRotation = FMath::RInterpTo(
		Controller->GetControlRotation(), TargetRotation, DeltaTime, LockOnCameraInterpSpeed);
	Controller->SetControlRotation(NewControllRotation);
}

void APlayerCharacter::StartSlowMotion()
{
	if (bEquipmentWheelSlowMotionActive)
	{
		return;
	}
	SavedGlobalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
	UGameplayStatics::SetGlobalTimeDilation(this, TimeDilation);

	bEquipmentWheelSlowMotionActive = true;
}

void APlayerCharacter::StopSlowMotion()
{
	if (!bEquipmentWheelSlowMotionActive)
	{
		return;
	}
	UGameplayStatics::SetGlobalTimeDilation(this, SavedGlobalTimeDilation);

	bEquipmentWheelSlowMotionActive = false;
	SavedGlobalTimeDilation = 1.f;
}

bool APlayerCharacter::InitializeDefaultAttributes()
{
	if (!ensureMsgf(
		AbilitySystemComponent != nullptr,
		TEXT("AbilitySystemComponent가 없습니다.")
	))
	{
		return false;
	}
	if (!ensureMsgf(
		AttributeSet != nullptr,
		TEXT("TinoAttributeSet이 없습니다.")
	))
	{
		return false;
	}
	if (!ensureMsgf(
		DefaultAttributesEffect != nullptr,
		TEXT("DefaultAttributesEffect가 지정되지 않았습니다.")
	))
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		DefaultAttributesEffect, 1.f, EffectContext);
	
	if (!ensureMsgf(
		EffectSpecHandle.IsValid(),
		TEXT("기본 능력치 Gameplay Effect Spec 생성에 실패했습니다.")
	))
	{
		return false;
	}
	
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
	
	return true;
}

void APlayerCharacter::RespawnAtInitialTransform()
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	ClearRespawnSequence();
	ReactionComponent->ResetDeathReaction();
	SetActorTransform(InitialSpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	if (AController* OwningController = GetController())
	{
		OwningController->SetControlRotation(InitialSpawnTransform.GetRotation().Rotator());
	}

	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	AttributeSet->SetStamina(AttributeSet->GetMaxStamina());

	if (!IsValid(RespawnSequence))
	{
		FinishRespawn(false);
		return;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.bDisableMovementInput = true;
	PlaybackSettings.bDisableLookAtInput = true;
	PlaybackSettings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceRestoreState;

	ALevelSequenceActor* NewSequenceActor = nullptr;
	ULevelSequencePlayer* NewSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), RespawnSequence, PlaybackSettings, NewSequenceActor);
	if (!IsValid(NewSequencePlayer) || !IsValid(NewSequenceActor))
	{
		if (IsValid(NewSequenceActor))
		{
			NewSequenceActor->Destroy();
		}
		FinishRespawn(false);
		return;
	}

	RespawnSequencePlayer = NewSequencePlayer;
	RespawnSequenceActor = NewSequenceActor;
	RespawnSequencePlayer->OnFinished.AddUniqueDynamic(
		this, &APlayerCharacter::HandleRespawnSequenceFinished);
	if (ATinoPlayerController* PlayerController = Cast<ATinoPlayerController>(GetController()))
	{
		PlayerController->SetPlayerUIVisible(false);
	}
	RespawnSequencePlayer->Play();
}

void APlayerCharacter::HandleRespawnSequenceFinished()
{
	if (RespawnSequencePlayer != nullptr)
	{
		RespawnSequencePlayer->OnFinished.RemoveDynamic(this, &APlayerCharacter::HandleRespawnSequenceFinished);
		RespawnSequencePlayer = nullptr;
	}
	if (IsValid(RespawnSequenceActor))
	{
		RespawnSequenceActor->Destroy();
	}
	RespawnSequenceActor = nullptr;
	if (ATinoPlayerController* PlayerController = Cast<ATinoPlayerController>(GetController()))
	{
		PlayerController->SetPlayerUIVisible(true);
	}
	FinishRespawn(true);
}

void APlayerCharacter::FinishRespawn(bool bFadeInFromBlack)
{
	// 시퀀스의 Transform/Animation 트랙이 남긴 값을 제거하고 정확한 부활 위치를 보장한다.
	SetActorTransform(InitialSpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CharacterStateComponent->RemoveStateTag(TinoGameplayTags::State_Dead);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	ConsumeMovementInputVector();
	bRunning = false;
	StaminaDelayTime = 0.f;
	bDeathHandled = false;
	UpdateRotationMode();
	UpdateMovementSpeed();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (bFadeInFromBlack && IsValid(PlayerController->PlayerCameraManager) && RespawnFadeInDuration > 0.f)
		{
			PlayerController->PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
			PlayerController->SetViewTarget(this);
			PlayerController->PlayerCameraManager->StartCameraFade(1.f, 0.f, RespawnFadeInDuration, FLinearColor::Black, false, false);
		}
		else
		{
			PlayerController->SetViewTarget(this);
		}
	}
}

void APlayerCharacter::ClearRespawnSequence()
{
	if (RespawnSequencePlayer != nullptr)
	{
		RespawnSequencePlayer->OnFinished.RemoveDynamic(this, &APlayerCharacter::HandleRespawnSequenceFinished);
		RespawnSequencePlayer->Stop();
		RespawnSequencePlayer = nullptr;
	}
	if (IsValid(RespawnSequenceActor))
	{
		RespawnSequenceActor->Destroy();
	}
	RespawnSequenceActor = nullptr;
	if (ATinoPlayerController* PlayerController = Cast<ATinoPlayerController>(GetController()))
	{
		PlayerController->SetPlayerUIVisible(true);
	}
}

float APlayerCharacter::ApplyDamageGameplayEffect(float DamageAmount, AController* EventInstigator,
                                                  AActor* DamageCauser)
{
	if (DamageAmount <= 0.f)
	{
		return 0.f;
	}
	if (!ensureMsgf(DamageEffect != nullptr, TEXT("DamageEffect가 지정되지 않았습니다.")))
	{
		return 0.f;
	}
	
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	AActor* InstigatorActor = DamageCauser;
	if (EventInstigator != nullptr && EventInstigator->GetPawn() != nullptr)
	{
		InstigatorActor = EventInstigator->GetPawn();
	}
	EffectContext.AddInstigator(InstigatorActor, DamageCauser);
	EffectContext.AddSourceObject(DamageCauser);

	FGameplayEffectSpecHandle EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		DamageEffect, 1.f, EffectContext);
	if (!ensureMsgf(
		EffectSpecHandle.IsValid(),TEXT("피해 Gameplay Effect Spec 생성에 실패했습니다.")))
	{
		return 0.f;
	}
	EffectSpecHandle.Data->SetSetByCallerMagnitude(TinoGameplayTags::Data_Damage, DamageAmount);
	
	const float PreviousHealth = AttributeSet->GetHealth();
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	return FMath::Max(PreviousHealth - AttributeSet->GetHealth(), 0.f);
}

void APlayerCharacter::HandleEquipmentChanged(UEquipmentLoadoutData* NewLoadout)
{
	// 공격 도중 장비가 변경되면 공격 데이터와 로드아웃 외형 & 콤보 공격 몽타주가 섞이지 않도록
	CombatComponent->CancelAttack();

	CombatComponent->SetEquippedAttackData(NewLoadout->AttackData.Get());
	CombatComponent->SetEquipmentWeaponActors(EquipmentComponent->GetRightHandEquipmentActor(),
		EquipmentComponent->GetLeftHandEquipmentActor());
	ReactionComponent->SetReactionSet(NewLoadout->Reactions);
}

void APlayerCharacter::HandleDeath(AActor* DamageCauser)
{
	if (bDeathHandled)
	{
		return;
	}
	bDeathHandled = true;
	StopAiming();
	TargetingComponent->ClearTarget();
	CharacterStateComponent->AddStateTag(TinoGameplayTags::State_Dead);
	// 마찬가지로 사망해도 장비창을 안전하게 닫기
	CancelEquipmentWheel();
	StopSlowMotion();

	StopRunning();
	CombatComponent->CancelAttack();
	DodgeComponent->CancelDodge();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ReactionComponent->PlayDeathReaction(DamageCauser);
	
	if (RespawnDelay <= 0.f)
	{
		RespawnAtInitialTransform();
		return;
	}
	
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this,
		&APlayerCharacter::RespawnAtInitialTransform, RespawnDelay, false);
}

void APlayerCharacter::HandleLockOnTargetChanged(AActor* PreviousTarget, AActor* NewTarget)
{
	UpdateRotationMode();
	bCameraTransition = true;

	const bool bLockedOn = NewTarget != nullptr;
	CameraBoom->CameraLagSpeed = bLockedOn ? LockOnCameraLagSpeed : DefaultCameraLagSpeed;
	CameraBoom->CameraLagMaxDistance = bLockedOn ? LockOnCameraLagMaxDistance : DefaultCameraLagMaxDistance;

	if (ATinoPlayerController* PlayerController = Cast<ATinoPlayerController>(GetController()))
	{
		PlayerController->SetLockOnMarkerTarget(NewTarget);
	}
}

float APlayerCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (DamageAmount <= 0.f || bDeathHandled)
	{
		return 0.f;
	}
	if (CharacterStateComponent->HasStateTag(TinoGameplayTags::State_Invincible))
	{
		return 0.f;
	}

	const float DamageToApply = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (DamageToApply <= 0.f)
	{
		return 0.f;
	}

	const float AppliedDamage = ApplyDamageGameplayEffect(DamageToApply, EventInstigator, DamageCauser);
	if (AppliedDamage <= 0.f)
	{
		return 0.f;
	}
	
	if (AttributeSet->GetHealth() <= 0.f)
	{
		HandleDeath(DamageCauser);
	}
	else
	{
		StopRunning();
		CombatComponent->CancelAttack();
		DodgeComponent->CancelDodge();
		ReactionComponent->PlayHitReaction(DamageCauser);

		if (IsValid(DamagedSound))
		{
			UGameplayStatics::PlaySoundAtLocation(this, DamagedSound, GetActorLocation());
		}
	}

	return AppliedDamage;
}
