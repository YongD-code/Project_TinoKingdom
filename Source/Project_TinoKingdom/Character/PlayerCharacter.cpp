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
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Project_TinoKingdom/Component/ReactionComponent.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/Component/TinoCombatComponent.h"
#include "Project_TinoKingdom/Component/TinoEquipmentComponent.h"
#include "Project_TinoKingdom/DataAsset/EquipmentLoadoutData.h"
#include "TinoNPCCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"

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
	ReactionComponent = CreateDefaultSubobject<UReactionComponent>(TEXT("ReactionComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	
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

	EquipmentComponent->OnEquipmentChanged.AddUniqueDynamic(this, &APlayerCharacter::HandleEquipmentChanged);
	// EquipmentComponent의 BeginPlay가 먼저 실행됐을 수 있기 때문에 현재 값도 직접 반영
	if (UEquipmentLoadoutData* CurrentLoadout = EquipmentComponent->GetCurrentLoadout())
	{
		HandleEquipmentChanged(CurrentLoadout);
	}
	
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

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 장비창이 열린 채 사망해도 전역 시간을 원래대로 복구
	StopSlowMotion();
	
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
	
	EnhancedInputComponent->BindAction(ToggleInventoryAction,ETriggerEvent::Started,this,&APlayerCharacter::ToggleInventory);
	
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::StartRunning);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRunning);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &APlayerCharacter::StopRunning);
	
	EnhancedInputComponent->BindAction(ToggleEquipmentMenuAction, ETriggerEvent::Started, this, &APlayerCharacter::OpenEquipmentWheel);
	EnhancedInputComponent->BindAction(ToggleEquipmentMenuAction, ETriggerEvent::Completed, this, &APlayerCharacter::ConfirmEquipmentWheel);
	EnhancedInputComponent->BindAction(ToggleEquipmentMenuAction, ETriggerEvent::Canceled, this, &APlayerCharacter::CancelEquipmentWheel);
	
	// EnhancedInputComponent->BindAction(ETriggerEvent::Started, this, &APlayerCharacter::DodgeAttack);
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// 공격 중에는 이동 입력을 받지 않는다.
	if (StatComponent->IsDead() || CombatComponent->IsAttacking() || ReactionComponent->IsReacting())
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
	if (StatComponent->IsDead() || CombatComponent->IsAttacking() || ReactionComponent->IsReacting())
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

ATinoNPCCharacter* APlayerCharacter::FindNearbyNPC() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}
	
	const FVector Center = GetActorLocation();
	const float Radius = 300.0f;
	
	TArray<FOverlapResult> Overlaps;
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NPCInterractionCheck), false, this);
	Params.AddIgnoredActor(this);
	
	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius),
		Params
	);
	
	DrawDebugSphere(World, Center, Radius, 24, FColor::Green, false, 1.0f);
	
	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Nearby NPC 없음"));
		return nullptr;
	}
	
	for(const FOverlapResult& Result : Overlaps)
	{
		ATinoNPCCharacter* NPC = Cast<ATinoNPCCharacter>(Result.GetActor());
		
		if (NPC)
		{
			UE_LOG(LogTemp, Warning, TEXT("Neary NPC 찾음: %s"), *NPC->GetName());
			return NPC;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Overlap은 됐지만 ATinoNPCCharacter가 없음"));
	return nullptr;
		
}

void APlayerCharacter::Attack()
{
	if (ATinoNPCCharacter* NearbyNPC = FindNearbyNPC())
	{
		NearbyNPC->StartDialogue();
		return;
	}
	if (StatComponent->IsDead() || ReactionComponent->IsReacting())
	{
		return;
	}
	CombatComponent->RequestAttack();
}

void APlayerCharacter::StartJump()
{
	if (StatComponent->IsDead() || CombatComponent->IsAttacking() || ReactionComponent->IsReacting())
	{
		return;
	}
	Jump();
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
	
	// 마찬가지로 사망해도 장비창을 안전하게 닫기
	CancelEquipmentWheel();
	StopSlowMotion();
	
	StopRunning();
	CombatComponent->CancelAttack();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	ReactionComponent->PlayDeathReaction(DamageCauser);
}

float APlayerCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (StatComponent != nullptr)
	{
		StatComponent->ApplyDamage(DamageAmount);
	}

	if (StatComponent->IsDead())
	{
		HandleDeath(DamageCauser);
	}
	else
	{
		StopRunning();
		CombatComponent->CancelAttack();
		ReactionComponent->PlayHitReaction(DamageCauser);
	}
	
	return AppliedDamage;
}
