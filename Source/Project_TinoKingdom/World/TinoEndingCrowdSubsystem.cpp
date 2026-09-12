#include "TinoEndingCrowdSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MassEntityConfigAsset.h"
#include "NavigationSystem.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "TinoEndingCrowdSpawner.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoEndingCrowdSession, Log, All);

bool UTinoEndingCrowdSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UTinoEndingCrowdSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTinoEndingCrowdSubsystem, STATGROUP_Tickables);
}

bool UTinoEndingCrowdSubsystem::ActivateEnding(ATinoEndingCrowdSpawner& SettingsSpawner,
	FName Tag, const FVector& ProjectionExtent, float Timeout, FString& OutError)
{
	if (bShuttingDown || Tag.IsNone() || SettingsSpawner.GetWorld() != GetWorld())
	{
		OutError = TEXT("엔딩 태그와 같은 월드의 설정 스포너를 확인하세요.");
		return false;
	}
	TSoftObjectPtr<UMassEntityConfigAsset> Config;
	if (!SettingsSpawner.GetEndingEntityConfig(Config))
	{
		OutError = TEXT("설정 스포너에 Entity Config를 하나 지정하고 자동 생성을 끄세요. 직접 생성한 스포너는 사용할 수 없습니다.");
		return false;
	}
	if (bEndingActive)
	{
		if (TargetTag == Tag && EntityConfig == Config)
		{
			return true;
		}
		OutError = TEXT("이 월드에는 이미 다른 설정의 엔딩 전환이 활성화되어 있습니다.");
		return false;
	}

	EntityConfig = Config;
	TargetTag = Tag;
	NavProjectionExtent = ProjectionExtent.ComponentMax(FVector::OneVector);
	ConversionTimeout = FMath::Max(Timeout, 1.0f);
	bEndingActive = true;
	bCollectingInitial = true;
	// 기존 대상은 한 번만 탐색하고, 이후 로딩은 몬스터의 플레이 시작 알림으로 처리합니다.
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->HasActorBegunPlay())
		{
			RegisterEnemy(**It);
		}
	}
	bCollectingInitial = false;
	UE_LOG(LogTinoEndingCrowdSession, Log, TEXT("엔딩 상태 활성화: 최초 대상 %d마리. 이후 로드되는 대상도 전환합니다."),
		InitialPendingCount);
	return true;
}

bool UTinoEndingCrowdSubsystem::ShouldConvert(const AEnemyCharacter& Enemy) const
{
	return bEndingActive && !bShuttingDown && Enemy.GetWorld() == GetWorld()
		&& Enemy.ActorHasTag(TargetTag) && !Enemy.IsDead();
}

void UTinoEndingCrowdSubsystem::RegisterEnemy(AEnemyCharacter& Enemy)
{
	if (!IsValid(&Enemy) || !ShouldConvert(Enemy))
	{
		return;
	}
	const TWeakObjectPtr<AEnemyCharacter> Key(&Enemy);
	if (const TSharedPtr<FConversion>* Existing = Conversions.Find(Key))
	{
		// 블루프린트의 플레이 시작 이후에도 표시와 AI 차단 상태를 다시 적용합니다.
		Enemy.SuppressForEndingCrowd((*Existing)->bHideWhileWaiting || (*Existing)->Phase == EConversionPhase::Ready);
		return;
	}
	if (Enemy.IsHidden())
	{
		return;
	}
	TSharedPtr<FConversion> Conversion = MakeShared<FConversion>();
	Conversion->Enemy = &Enemy;
	Conversion->PhaseStartTime = GetWorld()->GetTimeSeconds();
	Conversion->bInitialPending = bCollectingInitial;
	Conversion->bHideWhileWaiting = !bCollectingInitial;
	InitialPendingCount += Conversion->bInitialPending ? 1 : 0;
	Conversions.Add(Key, Conversion);
	// 늦게 로드된 몬스터는 사람이 준비될 때까지 화면과 전투에 등장하지 않습니다.
	Enemy.SuppressForEndingCrowd(Conversion->bHideWhileWaiting);
}

void UTinoEndingCrowdSubsystem::UnregisterEnemy(AEnemyCharacter& Enemy)
{
	TSharedPtr<FConversion> Conversion;
	if (Conversions.RemoveAndCopyValue(TWeakObjectPtr<AEnemyCharacter>(&Enemy), Conversion))
	{
		ReleaseConversion(*Conversion);
	}
}

void UTinoEndingCrowdSubsystem::ReleaseConversion(FConversion& Conversion)
{
	if (Conversion.bInitialPending)
	{
		--InitialPendingCount;
		Conversion.bInitialPending = false;
	}
	if (ATinoEndingCrowdSpawner* Spawner = Conversion.Spawner.Get())
	{
		Spawner->CancelEndingSpawn();
		Spawner->Destroy();
	}
	Conversion.Spawner.Reset();
	if (AEnemyCharacter* Enemy = Conversion.Enemy.Get(); Enemy && !Enemy->IsDead())
	{
		// 스트리밍이 같은 액터 객체를 재사용하는 경우에 대비해 저장된 원래 상태를 복원합니다.
		// 종료 중이므로 AI를 다시 실행하지 않습니다.
		Enemy->ReleaseEndingCrowdSuppression();
	}
}

void UTinoEndingCrowdSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bEndingActive || bShuttingDown)
	{
		return;
	}
	PollElapsed += DeltaTime;
	if (PollElapsed < 0.25f)
	{
		return;
	}
	PollElapsed = 0.0f;
	int32 InFlightCount = 0;
	TArray<TWeakObjectPtr<AEnemyCharacter>> Keys;
	Conversions.GetKeys(Keys);
	for (const auto& Pair : Conversions)
	{
		InFlightCount += Pair.Value->Phase == EConversionPhase::Spawning ? 1 : 0;
	}
	// 등록 해제나 외부 완료 이벤트가 발생해도 맵 순회가 깨지지 않도록 키를 복사합니다.
	for (const TWeakObjectPtr<AEnemyCharacter>& Key : Keys)
	{
		if (bShuttingDown)
		{
			return;
		}
		const TSharedPtr<FConversion> Conversion = Conversions.FindRef(Key);
		if (!Conversion)
		{
			continue;
		}
		if (!Key.IsValid() || !Key->HasActorBegunPlay() || Key->IsDead())
		{
			Conversions.Remove(Key);
			ReleaseConversion(*Conversion);
			continue;
		}
		ProcessConversion(*Conversion, InFlightCount);
	}
	if (!bShuttingDown && !bInitialCrowdFinished && InitialPendingCount == 0)
	{
		bInitialCrowdFinished = true;
		UE_LOG(LogTinoEndingCrowdSession, Log, TEXT("최초 로드 영역의 전환 처리 완료. 현재 군중 %d명."), GetReadyCount());
		OnInitialCrowdReady.Broadcast();
	}
}

void UTinoEndingCrowdSubsystem::ProcessConversion(FConversion& Conversion, int32& InFlightCount)
{
	AEnemyCharacter* Enemy = Conversion.Enemy.Get();
	const double Now = GetWorld()->GetTimeSeconds();
	if (Conversion.Phase == EConversionPhase::Ready)
	{
		if (!Conversion.Spawner.IsValid())
		{
			FailConversion(Conversion, TEXT("군중을 관리하던 런타임 스포너가 제거되었습니다."));
		}
		return;
	}
	if (Conversion.Phase == EConversionPhase::Failed)
	{
		return;
	}
	if (Conversion.Phase == EConversionPhase::Spawning)
	{
		ATinoEndingCrowdSpawner* Spawner = Conversion.Spawner.Get();
		if (Spawner && Spawner->HasFinishedPreparedSpawn())
		{
			--InFlightCount;
			if (Spawner->GetSpawnedEntityCount() != 1)
			{
				FailConversion(Conversion, TEXT("생성된 군중 수가 한 명이 아닙니다."));
				return;
			}
			Conversion.Phase = EConversionPhase::Ready;
			if (Conversion.bInitialPending)
			{
				--InitialPendingCount;
				Conversion.bInitialPending = false;
			}
			Enemy->SuppressForEndingCrowd(true);
			UE_LOG(LogTinoEndingCrowdSession, Log, TEXT("%s: 사람 군중 전환 완료."), *GetNameSafe(Enemy));
		}
		else if (!Spawner || Now - Conversion.PhaseStartTime >= ConversionTimeout)
		{
			--InFlightCount;
			FailConversion(Conversion, TEXT("군중 생성에 실패했거나 대기 시간을 초과했습니다."));
		}
		return;
	}

	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	FNavLocation Ground;
	if (!Navigation || !Movement || !Navigation->ProjectPointToNavigation(
		Movement->GetActorFeetLocation(), Ground, NavProjectionExtent))
	{
		if (Now - Conversion.PhaseStartTime >= ConversionTimeout)
		{
			FailConversion(Conversion, TEXT("발밑 NavMesh 준비 대기 시간을 초과했습니다. 이 지역의 내비게이션을 확인하세요."));
		}
		return;
	}
	// 한꺼번에 많은 비동기 생성 요청을 시작하지 않습니다. 대기열 자체에는 제한 시간을 적용하지 않습니다.
	if (InFlightCount >= 4)
	{
		return;
	}
	FActorSpawnParameters Parameters;
	Parameters.OverrideLevel = GetWorld()->PersistentLevel;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Parameters.ObjectFlags |= RF_Transient;
	const FTransform Transform(FRotator(0.0, Enemy->GetActorRotation().Yaw, 0.0), Ground.Location, FVector::OneVector);
	ATinoEndingCrowdSpawner* Spawner = GetWorld()->SpawnActor<ATinoEndingCrowdSpawner>(
		ATinoEndingCrowdSpawner::StaticClass(), Transform, Parameters);
	if (!Spawner)
	{
		FailConversion(Conversion, TEXT("런타임 군중 스포너를 만들지 못했습니다."));
		return;
	}
	Conversion.Spawner = Spawner;
	Spawner->SetEndingEntityConfig(EntityConfig);
	if (!Spawner->PrepareSpawnLocations(TArray<FTransform>{Transform}))
	{
		FailConversion(Conversion, TEXT("몬스터 위치의 군중 생성 데이터를 준비하지 못했습니다."));
		return;
	}
	Conversion.Phase = EConversionPhase::Spawning;
	Conversion.PhaseStartTime = Now;
	++InFlightCount;
	Spawner->SpawnPreparedCrowd();
}

void UTinoEndingCrowdSubsystem::FailConversion(FConversion& Conversion, const FString& Reason)
{
	Conversion.Phase = EConversionPhase::Failed;
	if (ATinoEndingCrowdSpawner* Spawner = Conversion.Spawner.Get())
	{
		Spawner->CancelEndingSpawn();
		Spawner->Destroy();
	}
	Conversion.Spawner.Reset();
	LastError = FString::Printf(TEXT("%s: %s"), *GetNameSafe(Conversion.Enemy.Get()), *Reason);
	UE_LOG(LogTinoEndingCrowdSession, Warning, TEXT("%s"), *LastError);
	// 엔딩 상태는 취소하지 않습니다. 늦게 로드된 적도 숨김과 전투 차단을 유지합니다.
	const FString ErrorToReport = LastError;
	OnConversionError.Broadcast(ErrorToReport);
}

void UTinoEndingCrowdSubsystem::RetryFailedConversions()
{
	LastError.Reset();
	for (auto& Pair : Conversions)
	{
		FConversion& Conversion = *Pair.Value;
		if (Conversion.Phase == EConversionPhase::Failed)
		{
			Conversion.Phase = EConversionPhase::WaitingForNavigation;
			Conversion.PhaseStartTime = GetWorld()->GetTimeSeconds();
		}
	}
}

int32 UTinoEndingCrowdSubsystem::GetReadyCount() const
{
	int32 Count = 0;
	for (const auto& Pair : Conversions)
	{
		Count += Pair.Value->Phase == EConversionPhase::Ready ? 1 : 0;
	}
	return Count;
}

int32 UTinoEndingCrowdSubsystem::GetPendingCount() const
{
	return Conversions.Num() - GetReadyCount();
}

void UTinoEndingCrowdSubsystem::Shutdown()
{
	if (bShuttingDown)
	{
		return;
	}
	bShuttingDown = true;
	bEndingActive = false;
	auto Remaining = MoveTemp(Conversions);
	Conversions.Empty();
	for (auto& Pair : Remaining)
	{
		ReleaseConversion(*Pair.Value);
	}
	OnInitialCrowdReady.Clear();
	OnConversionError.Clear();
}

void UTinoEndingCrowdSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
	Shutdown();
	Super::OnWorldEndPlay(InWorld);
}

void UTinoEndingCrowdSubsystem::Deinitialize()
{
	Shutdown();
	Super::Deinitialize();
}
