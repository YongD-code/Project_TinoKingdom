#include "EnemyCharacter.h"

#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BrainComponent.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"
#include "Project_TinoKingdom/Constants/TinoCollision.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/Component/PlayerProgressionComponent.h"
#include "Project_TinoKingdom/GameMode/TinoGameMode.h"
#include "Project_TinoKingdom/UI/EnemyHealthBarWidget.h"
#include "Project_TinoKingdom/World/TinoEndingCrowdSubsystem.h"

namespace
{
ECookingTag InferCookingTagFromDrop(const FName ItemId, const FText& ItemName)
{
	const FString IdString = ItemId.ToString();
	const FString NameString = ItemName.ToString();
	const FString SearchText = IdString + TEXT(" ") + NameString;

	if (SearchText.Contains(TEXT("Slime")) || SearchText.Contains(TEXT("슬라임")))
	{
		return ECookingTag::Slime;
	}
	if (SearchText.Contains(TEXT("WaterBest")) || SearchText.Contains(TEXT("Fish")) || SearchText.Contains(TEXT("Fin")) ||
		SearchText.Contains(TEXT("물짱")) || SearchText.Contains(TEXT("생선")) || SearchText.Contains(TEXT("지느러미")))
	{
		return ECookingTag::Fish;
	}
	if (SearchText.Contains(TEXT("Mushroom")) || SearchText.Contains(TEXT("버섯")))
	{
		return ECookingTag::Mushroom;
	}
	if (SearchText.Contains(TEXT("Meat")) || SearchText.Contains(TEXT("고기")))
	{
		return ECookingTag::Meat;
	}
	if (SearchText.Contains(TEXT("Herb")) || SearchText.Contains(TEXT("약초")))
	{
		return ECookingTag::Herb;
	}
	if (SearchText.Contains(TEXT("Wood")) || SearchText.Contains(TEXT("나무")))
	{
		return ECookingTag::Wood;
	}

	return ECookingTag::None;
}
}


AEnemyCharacter::AEnemyCharacter()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = true;
	MovementComponent->RotationRate = FRotator(0.f, 360.f, 0.f);
	MovementComponent->MaxWalkSpeed = WalkSpeed;

	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	LockOnAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("LockOnAnchor"));
	LockOnAnchor->SetupAttachment(GetRootComponent());

	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(GetRootComponent());
	HealthBarComponent->SetWidgetClass(UEnemyHealthBarWidget::StaticClass());
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawSize(HealthBarDrawSize);
	HealthBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HealthBarHeight));
	HealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBarComponent->SetVisibility(true);
	
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	PrimaryActorTick.bCanEverTick = true;
	
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("TinoCapsule"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

}

void AEnemyCharacter::BeginPlay()
{
	// 블루프린트의 플레이 시작 이벤트보다 먼저 엔딩 상태를 적용합니다.
	UTinoEndingCrowdSubsystem* EndingCrowd = GetWorld()->GetSubsystem<UTinoEndingCrowdSubsystem>();
	if (EndingCrowd)
	{
		EndingCrowd->RegisterEnemy(*this);
	}
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	LastSafeTransform = GetActorTransform();
	SetEnemyAIActive(false);

	GetWorldTimerManager().SetTimer(
		AIActivationTimerHandle,
		this,
		&AEnemyCharacter::UpdateAIActivation,
		FMath::Max(AIActivationCheckInterval, 0.05f),
		true);
	UpdateAIActivation();

	if (StatComponent != nullptr)
	{
		StatComponent->OnDead.AddUniqueDynamic(this, &AEnemyCharacter::HandleDead);
		StatComponent->OnHPChanged.AddUniqueDynamic(this, &AEnemyCharacter::HandleHPChanged);
		UpdateHealthBar(StatComponent->GetCurrentHP(), StatComponent->GetMaxHP());
	}
	if (EndingCrowd)
	{
		// 플레이 시작에서 추가한 태그와 블루프린트가 변경한 표시 상태도 반영합니다.
		EndingCrowd->RegisterEnemy(*this);
	}
}

void AEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTinoEndingCrowdSubsystem* EndingCrowd = GetWorld()->GetSubsystem<UTinoEndingCrowdSubsystem>())
	{
		EndingCrowd->UnregisterEnemy(*this);
	}
	SetEngaged(false);

	GetWorldTimerManager().ClearTimer(AttackResetTimerHandle);
	GetWorldTimerManager().ClearTimer(HitReactionResetTimerHandle);
	GetWorldTimerManager().ClearTimer(AIActivationTimerHandle);
	
	if (IsValid(StatComponent))
	{
		StatComponent->OnDead.RemoveDynamic(this, &AEnemyCharacter::HandleDead);
		StatComponent->OnHPChanged.RemoveDynamic(this, &AEnemyCharacter::HandleHPChanged);
	}
	Super::EndPlay(EndPlayReason);
}

float AEnemyCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	// 군중 전환 준비 중이거나 전환된 몬스터는 피해와 피격 반응을 받지 않습니다.
	if (!CanBeDamaged())
	{
		return 0.0f;
	}

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (StatComponent != nullptr)
	{
		LastDamageCauser = DamageCauser;

		if (EventInstigator != nullptr && EventInstigator->GetPawn() != nullptr)
		{
			LastDamageCauser = EventInstigator->GetPawn();
		}
		
		StatComponent->ApplyDamage(DamageAmount);

		if (!StatComponent->IsDead())
		{
			AActor* AggroTarget = DamageCauser;

			if (EventInstigator != nullptr && EventInstigator->GetPawn() != nullptr)
			{
				AggroTarget = EventInstigator->GetPawn();
			}

			SetAggroTarget(AggroTarget);

			ApplyKnockbackFrom(AggroTarget);
			PlayHitReaction();
		}
	}

	return AppliedDamage;
}

bool AEnemyCharacter::CanAttack() const
{
	if (!bAIActive || bAttacking || bHitReacting || bDead || bCinematicAIBlocked || bEndingCrowdSuppressed)
	{
		return false;
	}

	if (StatComponent != nullptr && StatComponent->IsDead())
	{
		return false;
	}

	if (AttackMontage == nullptr)
	{
		return false;
	}

	const USkeletalMeshComponent* MeshComponent = GetCombatAnimationMesh();
	if (MeshComponent == nullptr || MeshComponent->GetAnimInstance() == nullptr)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	return CurrentTime - LastAttackTime >= AttackCooldown;
}

bool AEnemyCharacter::IsTargetWithinAttackRange(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist2D(
		GetActorLocation(),
		TargetActor->GetActorLocation());

	// 공격 도중 거리가 달라져도 현재 몽타주가 끝날 때까지 공격 브랜치를 유지한다.
	// 특히 Dash의 Root Motion으로 350 아래에 들어갔을 때 Move To가 끼어드는 것을 막는다.
	if (bAttacking)
	{
		return true;
	}

	if (DistanceToTarget <= AttackRange)
	{
		return true;
	}

	// Dash 범위에 들어왔더라도 쿨다운 중이면 공격 브랜치로 전환하지 않고 계속 추적한다.
	return CanUseDashAttackAtDistance(DistanceToTarget) && CanAttack();
}

bool AEnemyCharacter::RequestAttack()
{
	if (!CanAttack())
	{
		return false;
	}

	USkeletalMeshComponent* AnimationMesh = GetCombatAnimationMesh();
	UAnimInstance* AnimInstance = AnimationMesh != nullptr ? AnimationMesh->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr || AttackMontage == nullptr)
	{
		CombatTarget = nullptr;
		return false;
	}

	bAttacking = true;
	LastAttackTime = GetWorld()->GetTimeSeconds();

	const FName SelectedSection = SelectAttackMontageSection();
	const float PlayLength = AnimInstance->Montage_Play(AttackMontage);
	if (PlayLength <= 0.f)
	{
		bAttacking = false;
		CombatTarget = nullptr;
		return false;
	}

	if (!SelectedSection.IsNone())
	{
		// 선택한 공격 한 개만 실행하고 다음 섹션으로 자동 연결되지 않게 한다.
		AnimInstance->Montage_SetNextSection(SelectedSection, NAME_None, AttackMontage);
		AnimInstance->Montage_JumpToSection(SelectedSection, AttackMontage);
		LastPlayedAttackSection = SelectedSection;
	}

	GetWorldTimerManager().ClearTimer(AttackResetTimerHandle);
	GetWorldTimerManager().SetTimer(
		AttackResetTimerHandle,
		this,
		&AEnemyCharacter::ResetAttackState,
		PlayLength + 0.2f,
		false
	);
	
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyCharacter::OnAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);

	GetCharacterMovement()->StopMovementImmediately();

	return true;
}

void AEnemyCharacter::HandleDead()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	bAIActive = false;

	// 쫓던 중에 죽으면 전투 카운트가 남으므로 여기서 정리한다.
	SetEngaged(false);
	bAttacking = false;
	bHitReacting = false;
	CombatTarget = nullptr;

	if (HealthBarComponent != nullptr)
	{
		HealthBarComponent->SetVisibility(false);
	}
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(LastDamageCauser))
	{
		if (UPlayerProgressionComponent* ProgressionComponent = PlayerCharacter->GetProgressionComponent())
		{
			ProgressionComponent->AddExperience(DropExperience);
		}
		if (!DropItemId.IsNone() && DropItemCount > 0)
		{
			if (UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
			{
				const ECookingTag DropCookingTag = InferCookingTagFromDrop(DropItemId, DropItemName);
				const bool bDropsMonkeyKey = DropItemId == UInventoryComponent::GetMonkeyKeyItemId();
				const EInventoryItemType DropItemType = bDropsMonkeyKey
					? EInventoryItemType::Key
					: (DropCookingTag == ECookingTag::None
						? EInventoryItemType::Etc
						: EInventoryItemType::Material);

				InventoryComponent->AddItem(
					DropItemId,
					DropItemName,
					DropItemCount,
					DropItemIcon,
					DropItemType,
					DropCookingTag
				);
			}
		}
	}
	
	if (AController* CurrentController = GetController())
	{
		if (AAIController* AIController = Cast<AAIController>(CurrentController))
		{
			AIController->StopMovement();
		}

		CurrentController->UnPossess();
	}

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	USkeletalMeshComponent* AnimationMesh = GetCombatAnimationMesh();
	UAnimInstance* AnimInstance = AnimationMesh != nullptr ? AnimationMesh->GetAnimInstance() : nullptr;
	if (AnimInstance != nullptr && DeathMontage != nullptr)
	{
		AnimInstance->Montage_Stop(0.1f);
		const float DeathLength = AnimInstance->Montage_Play(DeathMontage);
		SetLifeSpan(DeathLength > 0.0f ? DeathLength + 2.0f : DeadLifeSpan);
	}
	else
	{
		SetLifeSpan(DeadLifeSpan);
	}
}

void AEnemyCharacter::HandleHPChanged(float CurrentValue, float MaxValue)
{
	UpdateHealthBar(CurrentValue, MaxValue);
}

void AEnemyCharacter::UpdateHealthBar(float CurrentValue, float MaxValue)
{
	if (HealthBarComponent == nullptr)
	{
		return;
	}

	HealthBarComponent->SetDrawSize(HealthBarDrawSize);
	HealthBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HealthBarHeight));
	HealthBarComponent->InitWidget();

	UEnemyHealthBarWidget* HealthBarWidget =
		Cast<UEnemyHealthBarWidget>(HealthBarComponent->GetUserWidgetObject());
	if (HealthBarWidget == nullptr)
	{
		return;
	}

	const float HealthPercent = MaxValue > 0.0f ? CurrentValue / MaxValue : 0.0f;
	HealthBarWidget->SetHealthPercent(HealthPercent);
	UpdateHealthBarVisibility();
}

void AEnemyCharacter::UpdateHealthBarVisibility()
{
	if (HealthBarComponent == nullptr || StatComponent == nullptr)
	{
		return;
	}

	const bool bShouldShow =
		!bDead &&
		!StatComponent->IsDead() &&
		StatComponent->GetCurrentHP() > 0.0f &&
		IsHealthBarInVisibleRange();

	HealthBarComponent->SetVisibility(bShouldShow);
}

bool AEnemyCharacter::IsHealthBarInVisibleRange() const
{
	if (MaxHealthBarVisibleDistance <= 0.0f)
	{
		return true;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(PlayerPawn))
	{
		return false;
	}

	return FVector::DistSquared(PlayerPawn->GetActorLocation(), GetActorLocation())
		<= FMath::Square(MaxHealthBarVisibleDistance);
}

void AEnemyCharacter::PlayHitReaction()
{
	if (bDead)
	{
		return;
	}

	USkeletalMeshComponent* AnimationMesh = GetCombatAnimationMesh();
	UAnimInstance* AnimInstance = AnimationMesh != nullptr ? AnimationMesh->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr || HitMontage == nullptr)
	{
		return;
	}

	bAttacking = false;
	bHitReacting = true;

	GetWorldTimerManager().ClearTimer(AttackResetTimerHandle);
	
	if (AttackMontage != nullptr && AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		AnimInstance->Montage_Stop(0.1f, AttackMontage);
	}

	const float PlayLength = AnimInstance->Montage_Play(HitMontage);
	if (PlayLength <= 0.0f)
	{
		bHitReacting = false;
		return;
	}
	
	GetWorldTimerManager().ClearTimer(HitReactionResetTimerHandle);
	GetWorldTimerManager().SetTimer(
		HitReactionResetTimerHandle,
		this,
		&AEnemyCharacter::ResetHitReactionState,
		PlayLength + 0.2f,
		false
	);
	
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyCharacter::OnHitMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, HitMontage);
}

void AEnemyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == AttackMontage)
	{
		GetWorldTimerManager().ClearTimer(AttackResetTimerHandle);
		ResetAttackState();
	}
}

void AEnemyCharacter::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == HitMontage)
	{
		GetWorldTimerManager().ClearTimer(HitReactionResetTimerHandle);
		ResetHitReactionState();
	}
}

void AEnemyCharacter::PerformAttackTrace()
{
	// 이미 재생 중인 공격 몽타주의 알림도 엔딩 이후 피해를 발생시키지 못하게 합니다.
	if (bDead || bEndingCrowdSuppressed || bCinematicAIBlocked)
	{
		return;
	}

	if (CombatTarget == nullptr)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FVector AttackDirection = GetActorForwardVector();

	if (CombatTarget != nullptr)
	{
		AttackDirection = CombatTarget->GetActorLocation() - GetActorLocation();
		AttackDirection.Z = 0.0f;

		if (AttackDirection.IsNearlyZero())
		{
			AttackDirection = GetActorForwardVector();
		}
		else
		{
			AttackDirection.Normalize();
		}
	}

	const FVector Start = GetActorLocation() + AttackDirection * 20.0f;
	const FVector End = Start + AttackDirection * AttackTraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyAttackTrace), false, this);

	TArray<FHitResult> HitResults;
	World->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		TinoCollision::Action,
		FCollisionShape::MakeSphere(AttackTraceRadius),
		Params
	);
	
	const bool bHit = HitResults.Num() > 0;
	
	DrawDebugSphere(World, End, AttackTraceRadius, 16, bHit ? FColor::Red : FColor::Green, false, 1.0f);

	if (!bHit)
	{
		return;
	}

	AActor* SelectedHitActor = nullptr;

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();

		if (HitActor == nullptr || HitActor == this)
		{
			continue;
		}

		AEnemyCharacter* HitEnemy = Cast<AEnemyCharacter>(HitActor);
		if (HitEnemy != nullptr && !HitEnemy->IsDead())
		{
			SelectedHitActor = HitActor;
			break;
		}

		if (HitActor == CombatTarget && SelectedHitActor == nullptr)
		{
			SelectedHitActor = HitActor;
		}
	}

	if (SelectedHitActor == nullptr)
	{
		return;
	}

	UGameplayStatics::ApplyDamage(
		SelectedHitActor,
		AttackDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	if (AEnemyCharacter* HitEnemy = Cast<AEnemyCharacter>(SelectedHitActor))
	{
		if (!HitEnemy->IsDead())
		{
			SetAggroTarget(HitEnemy);
			HitEnemy->SetAggroTarget(this);
		}
	}
}

void AEnemyCharacter::SetCombatTarget(AActor* NewTarget)
{
	CombatTarget = NewTarget;
}

void AEnemyCharacter::SetCinematicAIBlocked(bool bBlocked)
{
	// 다른 시네마틱의 종료 이벤트가 엔딩 이후 몬스터의 AI를 다시 켜지 못하게 합니다.
	if (bEndingCrowdSuppressed && !bBlocked)
	{
		return;
	}
	if (bCinematicAIBlocked == bBlocked)
	{
		return;
	}

	bCinematicAIBlocked = bBlocked;

	if (bCinematicAIBlocked)
	{
		CombatTarget = nullptr;
		bAttacking = false;
		bHitReacting = false;
		LastAttackTime = -999.0f;
		SetEngaged(false);
		SetEnemyAIActive(false);

		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			if (UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent())
			{
				BlackboardComponent->ClearValue(AEnemyAIController::TargetPlayer);
			}
		}
		return;
	}

	UpdateAIActivation();
}

void AEnemyCharacter::SuppressForEndingCrowd(bool bHide)
{
	if (!bEndingCrowdSuppressed)
	{
		bBeforeEndingAIBlocked = bCinematicAIBlocked;
		bBeforeEndingHidden = IsHidden();
		bBeforeEndingCollision = GetActorEnableCollision();
		bBeforeEndingDamage = CanBeDamaged();
		bBeforeEndingTick = IsActorTickEnabled();
		bEndingCrowdSuppressed = true;
	}
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	SetCinematicAIBlocked(true);
	if (bHide)
	{
		SetActorHiddenInGame(true);
	}
}

void AEnemyCharacter::ReleaseEndingCrowdSuppression()
{
	if (!bEndingCrowdSuppressed)
	{
		return;
	}
	bEndingCrowdSuppressed = false;
	bCinematicAIBlocked = bBeforeEndingAIBlocked;
	SetActorHiddenInGame(bBeforeEndingHidden);
	SetActorEnableCollision(bBeforeEndingCollision);
	SetCanBeDamaged(bBeforeEndingDamage);
	SetActorTickEnabled(bBeforeEndingTick);
}

void AEnemyCharacter::SetEngaged(bool bNewEngaged)
{
	if (bEngaged == bNewEngaged)
	{
		return;
	}

	bEngaged = bNewEngaged;

	ATinoGameMode* GameMode = Cast<ATinoGameMode>(UGameplayStatics::GetGameMode(this));
	if (!IsValid(GameMode))
	{
		return;
	}

	if (bEngaged)
	{
		GameMode->NotifyEnemyEngaged();
	}
	else
	{
		GameMode->NotifyEnemyDisengaged();
	}
}

void AEnemyCharacter::RegisterCombatAnimationMesh(USkeletalMeshComponent* AnimationMesh)
{
	if (IsValid(AnimationMesh) && AnimationMesh->GetOwner() == this)
	{
		CombatAnimationMesh = AnimationMesh;
	}
}

void AEnemyCharacter::UnregisterCombatAnimationMesh(USkeletalMeshComponent* AnimationMesh)
{
	if (CombatAnimationMesh == AnimationMesh)
	{
		CombatAnimationMesh = nullptr;
	}
}

USkeletalMeshComponent* AEnemyCharacter::GetCombatAnimationMesh() const
{
	return IsValid(CombatAnimationMesh) ? CombatAnimationMesh.Get() : GetMesh();
}

bool AEnemyCharacter::HasAttackMontageSection(FName SectionName) const
{
	return AttackMontage != nullptr
		&& !SectionName.IsNone()
		&& AttackMontage->GetSectionIndex(SectionName) != INDEX_NONE;
}

bool AEnemyCharacter::CanUseDashAttackAtDistance(float DistanceToTarget) const
{
	if (!HasAttackMontageSection(DashAttackSection))
	{
		return false;
	}

	const float MinDistance = FMath::Max(DashAttackMinDistance, AttackRange);
	const float MaxDistance = FMath::Max(DashAttackMaxDistance, MinDistance);
	return DistanceToTarget >= MinDistance && DistanceToTarget <= MaxDistance;
}

FName AEnemyCharacter::SelectAttackMontageSection() const
{
	if (IsValid(CombatTarget))
	{
		const float DistanceToTarget = FVector::Dist2D(
			GetActorLocation(),
			CombatTarget->GetActorLocation());

		if (CanUseDashAttackAtDistance(DistanceToTarget))
		{
			return DashAttackSection;
		}
	}

	const FName AttackSections[] = {AttackSection1, AttackSection2, AttackSection3};
	TArray<FName, TInlineAllocator<3>> ValidSections;
	for (const FName SectionName : AttackSections)
	{
		if (HasAttackMontageSection(SectionName))
		{
			ValidSections.Add(SectionName);
		}
	}

	if (ValidSections.IsEmpty())
	{
		return NAME_None;
	}

	if (ValidSections.Num() == 1)
	{
		return ValidSections[0];
	}

	const float RepeatWeight = FMath::Clamp(RepeatAttackWeightMultiplier, 0.0f, 1.0f);
	float TotalWeight = 0.0f;
	for (const FName SectionName : ValidSections)
	{
		TotalWeight += SectionName == LastPlayedAttackSection ? RepeatWeight : 1.0f;
	}

	float SelectionValue = FMath::FRand() * TotalWeight;
	for (const FName SectionName : ValidSections)
	{
		const float SectionWeight = SectionName == LastPlayedAttackSection ? RepeatWeight : 1.0f;
		if (SelectionValue < SectionWeight)
		{
			return SectionName;
		}

		SelectionValue -= SectionWeight;
	}

	return ValidSections.Last();
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateHealthBarVisibility();

	if (bAIActive && GetCharacterMovement()->IsMovingOnGround())
	{
		LastSafeTransform = GetActorTransform();
	}

	if (!bAIActive || CombatTarget == nullptr || bDead)
	{
		return;
	}
	
	if (AEnemyCharacter* TargetEnemy = Cast<AEnemyCharacter>(CombatTarget))
	{
		if (TargetEnemy->IsDead())
		{
			CombatTarget = nullptr;
			return;
		}
	}
	
	FVector Direction = CombatTarget->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;
	
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = FRotator(0.0f, Direction.Rotation().Yaw, 0.0f);

	const FRotator NewRotation = FMath::RInterpConstantTo(
		CurrentRotation,
		TargetRotation,
		DeltaSeconds,
		AttackTurnSpeed
	);

	SetActorRotation(NewRotation);
}

void AEnemyCharacter::ApplyKnockbackFrom(AActor* DamageCauser)
{
	if (DamageCauser == nullptr || bDead)
	{
		return;
	}
	
	FVector KnockbackDirection = GetActorLocation() - DamageCauser->GetActorLocation();
	KnockbackDirection.Z = 0.0f;
	
	if (KnockbackDirection.IsNearlyZero())
	{
		KnockbackDirection = -GetActorForwardVector();
		
		
	}
	
	KnockbackDirection.Normalize();
	
	const FVector KnockbackVelocity = KnockbackDirection * KnockbackPower + FVector::UpVector * KnockbackUpPower;
	
	LaunchCharacter(KnockbackVelocity,true, true);
		
}

bool AEnemyCharacter::CanBeTargeted_Implementation() const
{
	// 전환 준비 중이거나 숨겨진 몬스터가 계속 잠금 대상으로 선택되는 것을 막습니다.
	return !IsDead() && !IsHidden() && CanBeDamaged();
}

FVector AEnemyCharacter::GetLockOnLocation_Implementation() const
{
	return LockOnAnchor->GetComponentLocation();
}

void AEnemyCharacter::ResetAttackState()
{
	bAttacking = false;
}

void AEnemyCharacter::ResetHitReactionState()
{
	bHitReacting = false;
}	

void AEnemyCharacter::SetAggroTarget(AActor* NewTarget)
{
	if (NewTarget == nullptr || NewTarget == this || bDead || bCinematicAIBlocked)
	{
		return;
	}

	CombatTarget = NewTarget;

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController == nullptr)
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent();
	if (BlackboardComponent == nullptr)
	{
		return;
	}

	BlackboardComponent->SetValueAsObject(
		AEnemyAIController::TargetPlayer,
		NewTarget
	);
}

void AEnemyCharacter::UpdateAIActivation()
{
	if (bDead || bCinematicAIBlocked)
	{
		if (bAIActive)
		{
			SetEnemyAIActive(false);
		}
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bPlayerIsNear = IsValid(PlayerPawn)
		&& FVector::DistSquared(PlayerPawn->GetActorLocation(), LastSafeTransform.GetLocation())
		<= FMath::Square(AIActivationDistance);

	if (bAIActive)
	{
		if (!bPlayerIsNear)
		{
			SetEnemyAIActive(false);
			return;
		}

		if (GetActorLocation().Z < LastSafeTransform.GetLocation().Z - MaxStreamingFallDistance)
		{
			SetActorTransform(LastSafeTransform, false, nullptr, ETeleportType::TeleportPhysics);
			SetEnemyAIActive(false);
		}
		return;
	}

	if (!bPlayerIsNear || !HasGroundBelow(LastSafeTransform.GetLocation()))
	{
		SetEnemyAIActive(false);
		return;
	}

	SetActorTransform(LastSafeTransform, false, nullptr, ETeleportType::TeleportPhysics);
	SetEnemyAIActive(true);
}

void AEnemyCharacter::SetEnemyAIActive(bool bEnabled)
{
	bAIActive = bEnabled;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		if (bEnabled)
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
			MovementComponent->MaxWalkSpeed = WalkSpeed;
		}
		else
		{
			MovementComponent->DisableMovement();
		}
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController == nullptr)
	{
		return;
	}

	if (!bEnabled)
	{
		AIController->StopMovement();
	}

	if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
	{
		if (bEnabled)
		{
			BrainComponent->ResumeLogic(TEXT("Enemy entered active range"));
		}
		else
		{
			BrainComponent->PauseLogic(TEXT("Enemy outside active range or waiting for streamed ground"));
		}
	}
}

bool AEnemyCharacter::HasGroundBelow(const FVector& Location) const
{
	const UWorld* World = GetWorld();
	const UCapsuleComponent* EnemyCapsule = GetCapsuleComponent();
	if (World == nullptr || EnemyCapsule == nullptr)
	{
		return false;
	}

	const FVector TraceStart = Location + FVector::UpVector * 10.0f;
	const FVector TraceEnd = Location - FVector::UpVector
		* (EnemyCapsule->GetScaledCapsuleHalfHeight() + GroundProbeDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyStreamedGroundProbe), false, this);
	return World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_WorldStatic,
		QueryParams);
}
