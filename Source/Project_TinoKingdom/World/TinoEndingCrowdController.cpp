#include "TinoEndingCrowdController.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "MassSpawner.h"

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
	UnbindSpawner();

	// 군중의 수명은 스포너가 관리하므로 제어 액터가 종료되어도 군중을 제거하지 않습니다.
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
		UE_LOG(LogTinoEndingCrowd, Warning,
			TEXT("%s: Assign a CrowdSpawner that has begun play in this world. Current: %s"),
			*GetName(), *GetNameSafe(Spawner));
		return;
	}

	GetWorldTimerManager().ClearTimer(TestSpawnTimerHandle);
	RequestedSpawner = Spawner;
	// DoSpawning 호출 도중 완료될 수 있으므로 중복 방지 상태와 완료 알림 연결을 먼저 설정합니다.
	bSpawnRequested = true;
	Spawner->OnSpawningFinishedEvent.AddUniqueDynamic(
		this, &ATinoEndingCrowdController::HandleSpawningFinished);

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

	bSpawningFinished = true;
	UnbindSpawner();
	UE_LOG(LogTinoEndingCrowd, Log,
		TEXT("%s: Crowd spawner reported spawning finished."), *GetName());
	OnCrowdSpawningFinished.Broadcast();
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
