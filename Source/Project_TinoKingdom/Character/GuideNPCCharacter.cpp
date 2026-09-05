#include "GuideNPCCharacter.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogGuideNPC, Log, All);

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

void AGuideNPCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindMoveCompleted();
	Super::EndPlay(EndPlayReason);
}

void AGuideNPCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bGuiding)
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

	MoveRequestElapsed += DeltaSeconds;
	if (!bMoveRequestActive && MoveRequestElapsed >= MoveRequestInterval)
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
	if (!IsValid(PlayerCharacter))
	{
		UE_LOG(LogGuideNPC, Warning, TEXT("%s 안내 시작 실패: 플레이어가 유효하지 않습니다."), *GetName());
		return;
	}

	if (GuideTargets.IsEmpty())
	{
		UE_LOG(LogGuideNPC, Warning, TEXT("%s 안내 시작 실패: GuideTargets가 비어 있습니다."), *GetName());
		return;
	}

	if (!IsGuideReady())
	{
		return;
	}

	GuidedPlayer = PlayerCharacter;
	CurrentTargetIndex = 0;
	bGuiding = true;
	bMoveRequestActive = false;
	MoveRequestElapsed = MoveRequestInterval;

	SetNPCMovementEnabled(true);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = GuideWalkSpeed;
	}

	BindMoveCompleted();
	UE_LOG(LogGuideNPC, Log, TEXT("%s 안내 시작: TargetCount %d"), *GetName(), GuideTargets.Num());
	MoveToCurrentTarget();
}

void AGuideNPCCharacter::StopGuide()
{
	if (AAIController* GuideController = Cast<AAIController>(GetController()))
	{
		GuideController->StopMovement();
	}

	bGuiding = false;
	bMoveRequestActive = false;
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
	AAIController* GuideController = ResolveGuideController();
	if (!IsValid(CurrentTarget) || GuideController == nullptr)
	{
		UE_LOG(LogGuideNPC, Warning,
			TEXT("%s 이동 요청 실패: Target %s, Controller %s"),
			*GetName(),
			*GetNameSafe(CurrentTarget),
			*GetNameSafe(GetController()));
		return;
	}

	FVector TargetLocation;
	if (!ResolveCurrentTargetLocation(TargetLocation))
	{
		UE_LOG(LogGuideNPC, Warning,
			TEXT("%s 이동 요청 실패: %s 위치를 NavMesh 위로 보정하지 못했습니다."),
			*GetName(),
			*GetNameSafe(CurrentTarget));
		return;
	}

	const EPathFollowingRequestResult::Type MoveResult = GuideController->MoveToLocation(
		TargetLocation,
		TargetAcceptanceRadius,
		true,
		true,
		true);

	if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		bMoveRequestActive = true;
		return;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		AdvanceGuideTarget();
		return;
	}

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogGuideNPC, Warning,
			TEXT("%s 이동 요청 실패: %s까지의 경로를 찾지 못했습니다."),
			*GetName(),
			*GetNameSafe(CurrentTarget));
	}
}

void AGuideNPCCharacter::AdvanceGuideTarget()
{
	bMoveRequestActive = false;
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

bool AGuideNPCCharacter::IsGuideReady() const
{
	if (GetCharacterMovement() == nullptr)
	{
		UE_LOG(LogGuideNPC, Warning, TEXT("%s 안내 시작 실패: CharacterMovement가 없습니다."), *GetName());
		return false;
	}

	return true;
}

bool AGuideNPCCharacter::ResolveCurrentTargetLocation(FVector& OutTargetLocation) const
{
	const AActor* CurrentTarget = GetCurrentTarget();
	if (!IsValid(CurrentTarget))
	{
		return false;
	}

	OutTargetLocation = CurrentTarget->GetActorLocation();

	const UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavigationSystem == nullptr || NavigationProjectionExtent <= 0.0f)
	{
		return true;
	}

	FNavLocation ProjectedLocation;
	const FVector ProjectionExtent(
		NavigationProjectionExtent,
		NavigationProjectionExtent,
		NavigationProjectionExtent);

	if (!NavigationSystem->ProjectPointToNavigation(OutTargetLocation, ProjectedLocation, ProjectionExtent))
	{
		return false;
	}

	OutTargetLocation = ProjectedLocation.Location;
	return true;
}

AAIController* AGuideNPCCharacter::ResolveGuideController()
{
	AAIController* GuideController = Cast<AAIController>(GetController());
	if (GuideController == nullptr)
	{
		SpawnDefaultController();
		GuideController = Cast<AAIController>(GetController());
	}

	return GuideController;
}

void AGuideNPCCharacter::BindMoveCompleted()
{
	AAIController* GuideController = ResolveGuideController();
	if (GuideController == nullptr || BoundGuideController == GuideController)
	{
		return;
	}

	UnbindMoveCompleted();
	BoundGuideController = GuideController;
	BoundGuideController->ReceiveMoveCompleted.AddUniqueDynamic(
		this,
		&AGuideNPCCharacter::HandleMoveCompleted);
}

void AGuideNPCCharacter::UnbindMoveCompleted()
{
	if (BoundGuideController != nullptr)
	{
		BoundGuideController->ReceiveMoveCompleted.RemoveDynamic(
			this,
			&AGuideNPCCharacter::HandleMoveCompleted);
		BoundGuideController = nullptr;
	}
}

void AGuideNPCCharacter::HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (!bGuiding)
	{
		return;
	}

	bMoveRequestActive = false;

	if (Result == EPathFollowingResult::Success)
	{
		AdvanceGuideTarget();
		return;
	}

	UE_LOG(LogGuideNPC, Warning,
		TEXT("%s 이동 완료 실패: TargetIndex %d, Result %d"),
		*GetName(),
		CurrentTargetIndex,
		static_cast<int32>(Result));
}

AActor* AGuideNPCCharacter::GetCurrentTarget() const
{
	return GuideTargets.IsValidIndex(CurrentTargetIndex) ? GuideTargets[CurrentTargetIndex].Get() : nullptr;
}
