#include "EnemyCharacter.h"

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
			PlayHitReaction();
		}
	}

	return AppliedDamage;
}

bool AEnemyCharacter::CanAttack() const
{
	if (bAttacking)
	{
		return false;
	}

	if (StatComponent != nullptr && StatComponent->IsDead())
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
		return false;
	}

	bAttacking = true;
	LastAttackTime = GetWorld()->GetTimeSeconds();

	const float PlayLength = AnimInstance->Montage_Play(AttackMontage);
	if (PlayLength <= 0.f)
	{
		bAttacking = false;
		return false;
	}

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
		SetLifeSpan(DeathLength > 0.0f ? DeathLength + 1.0f : DeadLifeSpan);
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
	CombatTarget = nullptr;

	if (AttackMontage != nullptr && AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		AnimInstance->Montage_Stop(0.1f, AttackMontage);
	}

	AnimInstance->Montage_Play(HitMontage);
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

	const FVector Forward = GetActorForwardVector();
	const FVector Start = GetActorLocation() + Forward * 50.0f;
	const FVector End = Start + Forward * AttackTraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyAttackTrace), false, this);

	FHitResult HitResult;
	const bool bHit = World->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		TinoCollision::Action,
		FCollisionShape::MakeSphere(AttackTraceRadius),
		Params
	);

	DrawDebugSphere(World, End, AttackTraceRadius, 16, bHit ? FColor::Red : FColor::Green, false, 1.0f);

	if (!bHit)
	{
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	if (HitActor == nullptr || HitActor == this)
	{
		return;
	}

	UGameplayStatics::ApplyDamage(
		HitActor,
		AttackDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);
}

void AEnemyCharacter::SetCombatTarget(AActor* NewTarget)
{
	CombatTarget = NewTarget;
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bAttacking || CombatTarget == nullptr || bDead)
	{
		return;
	}

	const FVector Direction = CombatTarget->GetActorLocation() - GetActorLocation();
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

void AEnemyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == AttackMontage)
	{
		bAttacking = false;
		CombatTarget = nullptr;
	}
}
