#include "TinoEndingCrowdSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MassEntityConfigAsset.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "TinoEndingCrowdSpawner.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoEndingCrowdSession, Log, All);

namespace
{
	// 한 카메라 컷의 일반적인 몬스터 수는 한 번에 처리하되 과도한 동시 생성을 제한합니다.
	constexpr int32 MaxConcurrentCrowdSpawns = 16;
}

bool UTinoEndingCrowdSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UTinoEndingCrowdSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTinoEndingCrowdSubsystem, STATGROUP_Tickables);
}

bool UTinoEndingCrowdSubsystem::ActivateEnding(ATinoEndingCrowdSpawner& SettingsSpawner,
	FName Tag, const FVector& ProjectionExtent, float Timeout, FString& OutError,
	UNiagaraSystem* InTransformationEffect, float InEffectToSpawnDelay,
	float InTransformationInterval, FName InGroupTag)
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
		if (TargetTag != Tag || EntityConfig != Config)
		{
			OutError = TEXT("이 월드에는 이미 다른 설정의 엔딩 전환이 활성화되어 있습니다.");
			return false;
		}
		if (bAllGroupsActive || (!InGroupTag.IsNone() && ActiveGroupTags.Contains(InGroupTag)))
		{
			return true;
		}
	}

	const bool bFirstActivation = !bEndingActive;
	if (bFirstActivation)
	{
		EntityConfig = Config;
		TargetTag = Tag;
		NavProjectionExtent = ProjectionExtent.ComponentMax(FVector::OneVector);
		ConversionTimeout = FMath::Max(Timeout, 1.0f);
		bEndingActive = true;
		bCollectingInitial = true;
	}
	if (InGroupTag.IsNone())
	{
		bAllGroupsActive = true;
	}
	else
	{
		ActiveGroupTags.Add(InGroupTag);
	}

	NavProjectionExtent = ProjectionExtent.ComponentMax(FVector::OneVector);
	ConversionTimeout = FMath::Max(Timeout, 1.0f);
	TransformationEffect = InTransformationEffect;
	EffectToSpawnDelay = FMath::Max(InEffectToSpawnDelay, 0.0f);
	TransformationInterval = FMath::Max(InTransformationInterval, 0.0f);
	NextEffectTime = GetWorld()->GetTimeSeconds();
	bCollectingCueTargets = true;
	const int32 PreviousConversionCount = Conversions.Num();
	// 새 카메라 이벤트가 올 때마다 이번 그룹에 속한 로드된 몬스터를 등록합니다.
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (It->HasActorBegunPlay())
		{
			RegisterEnemy(**It);
		}
	}
	bCollectingCueTargets = false;
	bCollectingInitial = false;
	UE_LOG(LogTinoEndingCrowdSession, Log,
		TEXT("엔딩 군중 그룹 활성화: 그룹=%s, 새 대상=%d마리. 이후 로드되는 같은 그룹 대상도 전환합니다."),
		InGroupTag.IsNone() ? TEXT("전체") : *InGroupTag.ToString(),
		Conversions.Num() - PreviousConversionCount);
	return true;
}

bool UTinoEndingCrowdSubsystem::ShouldConvert(const AEnemyCharacter& Enemy) const
{
	if (!bEndingActive || bShuttingDown || Enemy.GetWorld() != GetWorld()
		|| !Enemy.ActorHasTag(TargetTag) || Enemy.IsDead())
	{
		return false;
	}
	if (bAllGroupsActive)
	{
		return true;
	}
	for (const FName GroupTag : ActiveGroupTags)
	{
		if (Enemy.ActorHasTag(GroupTag))
		{
			return true;
		}
	}
	return false;
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
	Conversion->bHideWhileWaiting = !bCollectingCueTargets;
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

	if (Conversion.Phase == EConversionPhase::WaitingForNavigation)
	{
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
		Conversion.SpawnTransform = FTransform(
			FRotator(0.0, Enemy->GetActorRotation().Yaw, 0.0), Ground.Location, FVector::OneVector);
		Conversion.EffectLocation = Enemy->GetActorLocation();
		Conversion.Phase = EConversionPhase::WaitingForEffect;
		Conversion.PhaseStartTime = Now;
	}

	if (Conversion.Phase == EConversionPhase::WaitingForEffect)
	{
		// 간격이 0이면 같은 카메라 그룹의 연기를 한 처리 틱에 모두 시작합니다.
		if (Now < NextEffectTime)
		{
			return;
		}
		if (IsValid(TransformationEffect))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), TransformationEffect, Conversion.EffectLocation);
		}
		Conversion.Phase = EConversionPhase::EffectPlaying;
		Conversion.PhaseStartTime = Now;
		NextEffectTime = Now + TransformationInterval;
		UE_LOG(LogTinoEndingCrowdSession, Log, TEXT("%s: 사람 전환 연기를 재생했습니다."), *GetNameSafe(Enemy));
		if (EffectToSpawnDelay > 0.0f)
		{
			return;
		}
	}

	if (Conversion.Phase != EConversionPhase::EffectPlaying
		|| Now - Conversion.PhaseStartTime < EffectToSpawnDelay)
	{
		return;
	}
	if (InFlightCount >= MaxConcurrentCrowdSpawns)
	{
		return;
	}

	FActorSpawnParameters Parameters;
	Parameters.OverrideLevel = GetWorld()->PersistentLevel;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Parameters.ObjectFlags |= RF_Transient;
	ATinoEndingCrowdSpawner* Spawner = GetWorld()->SpawnActor<ATinoEndingCrowdSpawner>(
		ATinoEndingCrowdSpawner::StaticClass(), Conversion.SpawnTransform, Parameters);
	if (!Spawner)
	{
		FailConversion(Conversion, TEXT("런타임 군중 스포너를 만들지 못했습니다."));
		return;
	}
	Conversion.Spawner = Spawner;
	Spawner->SetEndingEntityConfig(EntityConfig);
	if (!Spawner->PrepareSpawnLocations(TArray<FTransform>{Conversion.SpawnTransform}))
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
			Conversion.SpawnTransform = FTransform::Identity;
			Conversion.EffectLocation = FVector::ZeroVector;
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
