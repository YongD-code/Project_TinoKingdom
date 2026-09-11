#include "TinoEndingCrowdController.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "MassSpawner.h"
#include "TinoEndingCrowdSpawner.h"
#include "TinoEndingCrowdSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoEndingCrowd, Log, All);

ATinoEndingCrowdController::ATinoEndingCrowdController()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")));
}

void ATinoEndingCrowdController::BeginPlay()
{
	Super::BeginPlay();
	if (bTransformTaggedMonsters)
	{
		if (UTinoEndingCrowdSubsystem* Session = GetWorld()->GetSubsystem<UTinoEndingCrowdSubsystem>();
			Session && Session->IsEndingActive())
		{
			// 관리 액터가 다시 로드되어도 이미 활성화된 월드 상태를 새 엔딩으로 시작하지 않습니다.
			BindSession(*Session);
			bSpawnRequested = true;
			bEndingWorldActive = true;
			bSpawningFinished = Session->HasInitialCrowdFinished();
			bMonstersTransformed = bSpawningFinished;
			LastSpawnError = Session->GetLastError();
			return;
		}
	}
	if (bTestSpawnOnBeginPlay)
	{
		if (TestSpawnDelay > 0.0f)
		{
			GetWorldTimerManager().SetTimer(TestSpawnTimerHandle, this,
				&ATinoEndingCrowdController::SpawnEndingCrowd, TestSpawnDelay, false);
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
	if (UTinoEndingCrowdSubsystem* Session = EndingSession.Get())
	{
		Session->OnInitialCrowdReady.RemoveAll(this);
		Session->OnConversionError.RemoveAll(this);
	}
	EndingSession.Reset();
	// 엔딩 상태와 군중 수명은 월드 관리자가 유지합니다. 제어 액터의 종료로 취소하지 않습니다.
	Super::EndPlay(EndPlayReason);
}

void ATinoEndingCrowdController::BindSession(UTinoEndingCrowdSubsystem& Session)
{
	Session.OnInitialCrowdReady.RemoveAll(this);
	Session.OnConversionError.RemoveAll(this);
	Session.OnInitialCrowdReady.AddUObject(this, &ATinoEndingCrowdController::HandleInitialCrowdReady);
	Session.OnConversionError.AddUObject(this, &ATinoEndingCrowdController::HandleSessionError);
	EndingSession = &Session;
}

void ATinoEndingCrowdController::SpawnEndingCrowd()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !HasActorBegunPlay() || bSpawnRequested)
	{
		return;
	}
	AMassSpawner* Spawner = CrowdSpawner.Get();
	if (!IsValid(Spawner) || Spawner->GetWorld() != World || !Spawner->HasActorBegunPlay())
	{
		FailSpawn(TEXT("현재 월드에서 플레이가 시작된 Crowd Spawner를 지정하세요."));
		return;
	}
	GetWorldTimerManager().ClearTimer(TestSpawnTimerHandle);
	LastSpawnError.Reset();
	bSpawnRequested = true;
	if (bTransformTaggedMonsters)
	{
		ATinoEndingCrowdSpawner* SettingsSpawner = Cast<ATinoEndingCrowdSpawner>(Spawner);
		UTinoEndingCrowdSubsystem* Session = World->GetSubsystem<UTinoEndingCrowdSubsystem>();
		if (!SettingsSpawner || !Session)
		{
			bSpawnRequested = false;
			FailSpawn(TEXT("몬스터 전환에는 TinoEndingCrowdSpawner와 게임 월드 관리자가 필요합니다."));
			return;
		}
		FString Error;
		if (!Session->ActivateEnding(*SettingsSpawner, TargetMonsterTag, NavProjectionExtent, TransformationTimeout, Error))
		{
			bSpawnRequested = false;
			FailSpawn(Error);
			return;
		}
		BindSession(*Session);
		bEndingWorldActive = true;
		if (Session->HasInitialCrowdFinished())
		{
			HandleInitialCrowdReady();
		}
		return;
	}

	// 기존의 단순 군중 생성 테스트 경로는 유지합니다.
	RequestedSpawner = Spawner;
	Spawner->OnSpawningFinishedEvent.AddUniqueDynamic(this, &ATinoEndingCrowdController::HandleSpawningFinished);
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
	OnCrowdSpawningFinished.Broadcast();
}

void ATinoEndingCrowdController::HandleInitialCrowdReady()
{
	if (bSpawningFinished)
	{
		return;
	}
	bSpawningFinished = true;
	bMonstersTransformed = true;
	UE_LOG(LogTinoEndingCrowd, Log, TEXT("최초 로드 대상의 엔딩 전환 처리 완료. 이후 로딩 대상은 계속 전환합니다."));
	OnCrowdSpawningFinished.Broadcast();
	if (IsValid(this) && HasActorBegunPlay())
	{
		OnMonstersTransformed.Broadcast();
	}
}

void ATinoEndingCrowdController::HandleSessionError(const FString& Reason)
{
	FailSpawn(Reason);
}

void ATinoEndingCrowdController::FailSpawn(const FString& Reason)
{
	LastSpawnError = Reason;
	UE_LOG(LogTinoEndingCrowd, Warning, TEXT("%s: %s"), *GetName(), *Reason);
	OnCrowdSpawnFailed.Broadcast(Reason);
}

void ATinoEndingCrowdController::RetryFailedCrowdConversions()
{
	if (UTinoEndingCrowdSubsystem* Session = EndingSession.Get())
	{
		LastSpawnError.Reset();
		Session->RetryFailedConversions();
	}
}

int32 ATinoEndingCrowdController::GetLoadedCrowdCount() const
{
	const UTinoEndingCrowdSubsystem* Session = EndingSession.Get();
	return Session ? Session->GetReadyCount() : 0;
}

int32 ATinoEndingCrowdController::GetPendingCrowdCount() const
{
	const UTinoEndingCrowdSubsystem* Session = EndingSession.Get();
	return Session ? Session->GetPendingCount() : 0;
}

void ATinoEndingCrowdController::UnbindSpawner()
{
	if (AMassSpawner* Spawner = RequestedSpawner.Get())
	{
		Spawner->OnSpawningFinishedEvent.RemoveDynamic(this, &ATinoEndingCrowdController::HandleSpawningFinished);
	}
	RequestedSpawner.Reset();
}
