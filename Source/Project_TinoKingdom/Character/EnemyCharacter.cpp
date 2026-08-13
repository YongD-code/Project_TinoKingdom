#include "EnemyCharacter.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Project_TinoKingdom/AI/EnemyAIController.h"
#include "Project_TinoKingdom/Constants/TinoCollision.h"


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
	
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	PrimaryActorTick.bCanEverTick = true;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	if (StatComponent != nullptr)
	{
		StatComponent->OnDead.AddDynamic(this, &AEnemyCharacter::HandleDead);
	}
}

float AEnemyCharacter::TakeDamage(
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
	if (bAttacking || bHitReacting || bDead)
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

	const USkeletalMeshComponent* MeshComponent = GetMesh();
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

bool AEnemyCharacter::RequestAttack()
{
	if (!CanAttack())
	{
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh() != nullptr ? GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr || AttackMontage == nullptr)
	{
		CombatTarget = nullptr;
		return false;
	}

	bAttacking = true;
	LastAttackTime = GetWorld()->GetTimeSeconds();

	const float PlayLength = AnimInstance->Montage_Play(AttackMontage);
	if (PlayLength <= 0.f)
	{
		bAttacking = false;
		CombatTarget = nullptr;
		return false;
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
	bAttacking = false;
	bHitReacting = false;
	CombatTarget = nullptr;
	
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

	UAnimInstance* AnimInstance = GetMesh() != nullptr ? GetMesh()->GetAnimInstance() : nullptr;
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

void AEnemyCharacter::PlayHitReaction()
{
	if (bDead)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() != nullptr ? GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr || HitMontage == nullptr)
	{
		return;
	}

	bAttacking = false;
	bHitReacting = true;
	CombatTarget = nullptr;

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
	if (bDead)
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

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f) + AttackDirection * 20.0f;
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

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor == nullptr || HitActor == this)
		{
			continue;
		}

		UGameplayStatics::ApplyDamage(
			HitActor,
			AttackDamage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);
		
		if (AEnemyCharacter* HitEnemy = Cast<AEnemyCharacter>(HitActor))
		{
			if (!HitEnemy->IsDead())
			{
				SetAggroTarget(HitEnemy);
				HitEnemy->SetAggroTarget(this);
			}
		}
	}
}

void AEnemyCharacter::SetCombatTarget(AActor* NewTarget)
{
	CombatTarget = NewTarget;
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CombatTarget == nullptr || bDead)
	{
		return;
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
	if (NewTarget == nullptr || NewTarget == this || bDead)
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