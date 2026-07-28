#include "EnemyCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Project_TinoKingdom/Component/StatComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = true;
	MovementComponent->RotationRate = FRotator(0.f, 360.f, 0.f);
	MovementComponent->MaxWalkSpeed = WalkSpeed;

	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
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

void AEnemyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == AttackMontage)
	{
		bAttacking = false;
	}
}

void AEnemyCharacter::HandleDead()
{
	if (AController* CurrentController = GetController())
	{
		CurrentController->StopMovement();
		CurrentController->UnPossess();
	}

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetLifeSpan(DeadLifeSpan);
}