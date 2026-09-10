#include "TinoEndingCrowdController.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MassSpawner.h"
#include "NavigationSystem.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "TinoEndingCrowdSpawner.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoEndingCrowd, Log, All);

ATinoEndingCrowdController::ATinoEndingCrowdController()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")));
}

void ATinoEndingCrowdController::BeginPlay()
{
	Super::BeginPlay();

	if (bTestSpawnOnBeginPlay)
	{
		// 지연이 0이어도 다른 레벨 액터의 플레이 시작이 끝나도록 다음 틱까지 기다립니다.
		if (TestSpawnDelay > 0.0f)
		{
			GetWorldTimerManager().SetTimer(
				TestSpawnTimerHandle, this, &ATinoEndingCrowdController::SpawnEndingCrowd,
				TestSpawnDelay, false);
		}
		else
		{
			TestSpawnTimerHandle = GetWorldTimerManager().SetTimerForNextTick(
				this, &ATinoEndingCrowdController::SpawnEndingCrowd);
		}
	}
}

void ATinoEndingCrowdController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TestSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(TransformationTimeoutHandle);
	GetWorldTimerManager().ClearTimer(FinalizeTransformationHandle);
	ATinoEndingCrowdSpawner* EndingSpawner = Cast<ATinoEndingCrowdSpawner>(RequestedSpawner.Get());
	UnbindSpawner();
	if (bTransformationInProgress)
	{
		if (IsValid(EndingSpawner))
		{
			EndingSpawner->CancelEndingSpawn();
		}
		RestorePendingMonsters();
		bTransformationInProgress = false;
	}

	// 전환이 완료된 군중은 스포너가 수명을 관리하므로 여기서 제거하지 않습니다.
	Super::EndPlay(EndPlayReason);
}

void ATinoEndingCrowdController::SpawnEndingCrowd()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !HasActorBegunPlay())
	{
		UE_LOG(LogTinoEndingCrowd, Warning,
			TEXT("%s: SpawnEndingCrowd must be called during gameplay."), *GetName());
		return;
	}

	if (bSpawnRequested)
	{
		UE_LOG(LogTinoEndingCrowd, Verbose,
			TEXT("%s: Ignoring duplicate crowd spawn request."), *GetName());
		return;
	}

	AMassSpawner* Spawner = CrowdSpawner.Get();
	if (!IsValid(Spawner) || Spawner->GetWorld() != World || !Spawner->HasActorBegunPlay())
	{
		FailSpawn(FString::Printf(TEXT("현재 월드에서 플레이가 시작된 CrowdSpawner를 지정하세요. 현재 값: %s"),
			*GetNameSafe(Spawner)));
		return;
	}

	LastSpawnError.Reset();
	// 몬스터의 AI 상태 변경 중 다른 이벤트가 호출되어도 같은 요청을 다시 시작하지 않습니다.
	bSpawnRequested = true;
	RequestedSpawner = Spawner;
	if (bTransformTaggedMonsters)
	{
		ATinoEndingCrowdSpawner* EndingSpawner = Cast<ATinoEndingCrowdSpawner>(Spawner);
		if (!EndingSpawner)
		{
			FailSpawn(TEXT("몬스터 전환에는 TinoEndingCrowdSpawner를 CrowdSpawner로 지정해야 합니다."));
			return;
		}
		if (!PrepareMonsterTransformation(*EndingSpawner))
		{
			return;
		}
	}

	GetWorldTimerManager().ClearTimer(TestSpawnTimerHandle);
	// DoSpawning 호출 도중 완료될 수 있으므로 완료 알림을 먼저 연결합니다.
	Spawner->OnSpawningFinishedEvent.AddUniqueDynamic(
		this, &ATinoEndingCrowdController::HandleSpawningFinished);
	if (bTransformationInProgress)
	{
		GetWorldTimerManager().SetTimer(TransformationTimeoutHandle, this,
			&ATinoEndingCrowdController::HandleTransformationTimeout,
			FMath::Max(TransformationTimeout, 1.0f), false);
	}

	UE_LOG(LogTinoEndingCrowd, Log,
		TEXT("%s: Requesting crowd spawn from %s (Count=%d, Scale=%.2f)."),
		*GetName(), *Spawner->GetName(), Spawner->GetCount(), Spawner->GetSpawningCountScale());
	Spawner->DoSpawning();
}

void ATinoEndingCrowdController::HandleSpawningFinished()
{
	if (!bSpawnRequested || bSpawningFinished)
	{
		return;
	}

	if (bTransformationInProgress)
	{
		// 스포너 내부의 완료 처리와 엔티티 목록 갱신이 끝난 다음 확인합니다.
		if (!FinalizeTransformationHandle.IsValid())
		{
			FinalizeTransformationHandle = GetWorldTimerManager().SetTimerForNextTick(
				this, &ATinoEndingCrowdController::FinalizeMonsterTransformation);
		}
		return;
	}

	bSpawningFinished = true;
	UnbindSpawner();
	UE_LOG(LogTinoEndingCrowd, Log,
		TEXT("%s: Crowd spawner reported spawning finished."), *GetName());
	OnCrowdSpawningFinished.Broadcast();
}

bool ATinoEndingCrowdController::PrepareMonsterTransformation(ATinoEndingCrowdSpawner& Spawner)
{
	if (TargetMonsterTag.IsNone())
	{
		FailSpawn(TEXT("Target Monster Tag를 지정하세요."));
		return false;
	}

	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Navigation)
	{
		FailSpawn(TEXT("현재 월드에 내비게이션 시스템이 없습니다."));
		return false;
	}

	TArray<AEnemyCharacter*> Targets;
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		AEnemyCharacter* Enemy = *It;
		if (IsValid(Enemy) && Enemy->HasActorBegunPlay() && Enemy->ActorHasTag(TargetMonsterTag)
			&& !Enemy->IsDead() && !Enemy->IsHidden())
		{
			Targets.Add(Enemy);
		}
	}
	if (Targets.IsEmpty())
	{
		FailSpawn(FString::Printf(TEXT("현재 로드된 월드에 %s 태그가 붙은 살아 있는 몬스터가 없습니다."),
			*TargetMonsterTag.ToString()));
		return false;
	}
	Targets.Sort([](const AEnemyCharacter& Left, const AEnemyCharacter& Right)
	{
		return Left.GetPathName() < Right.GetPathName();
	});

	TArray<FTransform> Transforms;
	Transforms.Reserve(Targets.Num());
	for (AEnemyCharacter* Enemy : Targets)
	{
		const UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
		FNavLocation Ground;
		if (!Movement || !Navigation->ProjectPointToNavigation(
			Movement->GetActorFeetLocation(), Ground, NavProjectionExtent))
		{
			FailSpawn(FString::Printf(TEXT("%s 발밑의 NavMesh를 찾지 못했습니다. 몬스터는 전환하지 않습니다."),
				*Enemy->GetName()));
			return false;
		}
		// 몬스터의 캡슐 중심 높이와 크기를 사람에게 복사하지 않습니다.
		Transforms.Emplace(FRotator(0.0, Enemy->GetActorRotation().Yaw, 0.0), Ground.Location, FVector::OneVector);
	}
	if (!Spawner.PrepareSpawnLocations(Transforms))
	{
		FailSpawn(TEXT("전용 스포너에 유효한 Entity Config를 하나 지정하고 자동 생성을 꺼주세요. 이미 사용 중인 스포너는 재사용할 수 없습니다."));
		return false;
	}

	PendingMonsters.Reserve(Targets.Num());
	for (AEnemyCharacter* Enemy : Targets)
	{
		FPendingMonster& Pending = PendingMonsters.AddDefaulted_GetRef();
		Pending.Enemy = Enemy;
		Pending.bWasAIBlocked = Enemy->IsCinematicAIBlocked();
		Pending.bCouldBeDamaged = Enemy->CanBeDamaged();
	}
	bTransformationInProgress = true;
	for (AEnemyCharacter* Enemy : Targets)
	{
		if (!IsValid(Enemy) || Enemy->IsDead())
		{
			FailSpawn(TEXT("AI 정지 도중 대상 몬스터가 사라지거나 사망하여 전환을 취소합니다."));
			return false;
		}
		Enemy->SetCanBeDamaged(false);
		Enemy->SetCinematicAIBlocked(true);
		if (!IsValid(this) || !HasActorBegunPlay() || !bTransformationInProgress)
		{
			return false;
		}
	}
	return true;
}

void ATinoEndingCrowdController::FinalizeMonsterTransformation()
{
	FinalizeTransformationHandle.Invalidate();
	if (!bTransformationInProgress)
	{
		return;
	}

	ATinoEndingCrowdSpawner* Spawner = Cast<ATinoEndingCrowdSpawner>(RequestedSpawner.Get());
	if (!IsValid(Spawner) || PendingMonsters.IsEmpty()
		|| Spawner->GetSpawnedEntityCount() != PendingMonsters.Num())
	{
		FailSpawn(TEXT("생성된 군중 수와 전환할 몬스터 수가 달라 전환을 취소합니다."));
		return;
	}
	for (const FPendingMonster& Pending : PendingMonsters)
	{
		if (!Pending.Enemy.IsValid() || Pending.Enemy->IsDead())
		{
			FailSpawn(TEXT("군중 준비 도중 대상 몬스터가 사라지거나 사망하여 전환을 취소합니다."));
			return;
		}
	}

	const int32 ConvertedCount = PendingMonsters.Num();
	for (const FPendingMonster& Pending : PendingMonsters)
	{
		AEnemyCharacter* Enemy = Pending.Enemy.Get();
		// 사망 처리를 호출하지 않아 경험치, 드롭, 처치 퀘스트가 발생하지 않습니다.
		Enemy->SetActorEnableCollision(false);
		Enemy->SetActorHiddenInGame(true);
		Enemy->SetActorTickEnabled(false);
	}
	PendingMonsters.Reset();
	bTransformationInProgress = false;
	bMonstersTransformed = true;
	bSpawningFinished = true;
	GetWorldTimerManager().ClearTimer(TransformationTimeoutHandle);
	UnbindSpawner();
	UE_LOG(LogTinoEndingCrowd, Log, TEXT("%s: 몬스터 %d마리를 사람 군중으로 전환했습니다."),
		*GetName(), ConvertedCount);
	OnCrowdSpawningFinished.Broadcast();
	if (IsValid(this) && HasActorBegunPlay())
	{
		OnMonstersTransformed.Broadcast();
	}
}

void ATinoEndingCrowdController::HandleTransformationTimeout()
{
	if (bTransformationInProgress)
	{
		FailSpawn(TEXT("군중 생성 대기 시간을 초과하여 전환을 취소합니다. 로딩 상태와 스포너 설정을 확인하세요."));
	}
}

void ATinoEndingCrowdController::FailSpawn(const FString& Reason)
{
	LastSpawnError = Reason;
	GetWorldTimerManager().ClearTimer(TestSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(TransformationTimeoutHandle);
	GetWorldTimerManager().ClearTimer(FinalizeTransformationHandle);
	// 생성 취소 중 완료 알림이 오더라도 다시 전환하지 않도록 연결부터 해제합니다.
	AMassSpawner* Spawner = RequestedSpawner.Get();
	UnbindSpawner();
	if (bTransformationInProgress)
	{
		if (ATinoEndingCrowdSpawner* EndingSpawner = Cast<ATinoEndingCrowdSpawner>(Spawner))
		{
			EndingSpawner->CancelEndingSpawn();
		}
		RestorePendingMonsters();
		bTransformationInProgress = false;
	}
	UE_LOG(LogTinoEndingCrowd, Warning, TEXT("%s: %s"), *GetName(), *Reason);
	// 요청이 시작된 뒤 실패한 경우 중복 방지 상태는 유지합니다. 재시도는 새 플레이에서 진행합니다.
	OnCrowdSpawnFailed.Broadcast(Reason);
}

void ATinoEndingCrowdController::RestorePendingMonsters()
{
	for (const FPendingMonster& Pending : PendingMonsters)
	{
		if (AEnemyCharacter* Enemy = Pending.Enemy.Get())
		{
			Enemy->SetCanBeDamaged(Pending.bCouldBeDamaged);
			if (!Enemy->IsDead())
			{
				Enemy->SetCinematicAIBlocked(Pending.bWasAIBlocked);
			}
		}
	}
	PendingMonsters.Reset();
}

void ATinoEndingCrowdController::UnbindSpawner()
{
	if (AMassSpawner* Spawner = RequestedSpawner.Get())
	{
		Spawner->OnSpawningFinishedEvent.RemoveDynamic(
			this, &ATinoEndingCrowdController::HandleSpawningFinished);
	}
	RequestedSpawner.Reset();
}
