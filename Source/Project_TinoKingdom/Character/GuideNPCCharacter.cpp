#include "GuideNPCCharacter.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"

AGuideNPCCharacter::AGuideNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AGuideNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = GuideWalkSpeed;
	}
}

void AGuideNPCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bGuiding || IsDead())
	{
		return;
	}

	AActor* CurrentTarget = GetCurrentTarget();
	if (!IsValid(CurrentTarget))
	{
		StopGuide();
		return;
	}

	if (!IsPlayerCloseEnough())
	{
		if (AAIController* GuideController = Cast<AAIController>(GetController()))
		{
			GuideController->StopMovement();
		}
		return;
	}

	const float AcceptanceRadiusSquared = FMath::Square(TargetAcceptanceRadius);
	if (FVector::DistSquared(GetActorLocation(), CurrentTarget->GetActorLocation()) <= AcceptanceRadiusSquared)
	{
		AdvanceGuideTarget();
		return;
	}

	MoveRequestElapsed += DeltaSeconds;
	if (MoveRequestElapsed >= MoveRequestInterval)
	{
		MoveToCurrentTarget();
	}
}

void AGuideNPCCharacter::OnDialogueCompleted_Implementation(APlayerCharacter* PlayerCharacter)
{
	Super::OnDialogueCompleted_Implementation(PlayerCharacter);

	if (bStartGuideAfterDialogue)
	{
		StartGuide(PlayerCharacter);
	}
}

void AGuideNPCCharacter::StartGuide(APlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter) || GuideTargets.IsEmpty() || IsDead())
	{
		return;
	}

	GuidedPlayer = PlayerCharacter;
	CurrentTargetIndex = 0;
	bGuiding = true;
	MoveRequestElapsed = MoveRequestInterval;

	SetNPCMovementEnabled(true);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = GuideWalkSpeed;
	}

	MoveToCurrentTarget();
}

void AGuideNPCCharacter::StopGuide()
{
	if (AAIController* GuideController = Cast<AAIController>(GetController()))
	{
		GuideController->StopMovement();
	}

	bGuiding = false;
	CurrentTargetIndex = INDEX_NONE;
	GuidedPlayer = nullptr;
	MoveRequestElapsed = 0.0f;
	SetNPCMovementEnabled(false);
}

void AGuideNPCCharacter::MoveToCurrentTarget()
{
	MoveRequestElapsed = 0.0f;

	if (!IsPlayerCloseEnough())
	{
		return;
	}

	AActor* CurrentTarget = GetCurrentTarget();
	AAIController* GuideController = Cast<AAIController>(GetController());
	if (!IsValid(CurrentTarget) || GuideController == nullptr)
	{
		return;
	}

	GuideController->MoveToActor(
		CurrentTarget,
		TargetAcceptanceRadius,
		true,
		true,
		true,
		nullptr,
		true);
}

void AGuideNPCCharacter::AdvanceGuideTarget()
{
	++CurrentTargetIndex;

	if (GuideTargets.IsValidIndex(CurrentTargetIndex))
	{
		MoveRequestElapsed = MoveRequestInterval;
		MoveToCurrentTarget();
		return;
	}

	if (bLoopGuide && !GuideTargets.IsEmpty())
	{
		CurrentTargetIndex = 0;
		MoveRequestElapsed = MoveRequestInterval;
		MoveToCurrentTarget();
		return;
	}

	StopGuide();
}

bool AGuideNPCCharacter::IsPlayerCloseEnough() const
{
	if (PlayerWaitDistance <= 0.0f)
	{
		return true;
	}

	if (!IsValid(GuidedPlayer))
	{
		return false;
	}

	return FVector::DistSquared(GuidedPlayer->GetActorLocation(), GetActorLocation())
		<= FMath::Square(PlayerWaitDistance);
}

AActor* AGuideNPCCharacter::GetCurrentTarget() const
{
	return GuideTargets.IsValidIndex(CurrentTargetIndex) ? GuideTargets[CurrentTargetIndex].Get() : nullptr;
}
