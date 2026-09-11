#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TinoEndingCrowdSubsystem.generated.h"

class AEnemyCharacter;
class ATinoEndingCrowdSpawner;
class UMassEntityConfigAsset;

DECLARE_MULTICAST_DELEGATE_OneParam(FTinoEndingCrowdSessionError, const FString&);

// 월드가 살아 있는 동안 엔딩 상태를 유지합니다. 셀 언로드와 무관하며 맵 재시작 시 초기화됩니다.
UCLASS()
class PROJECT_TINOKINGDOM_API UTinoEndingCrowdSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	bool ActivateEnding(ATinoEndingCrowdSpawner& SettingsSpawner, FName Tag,
		const FVector& ProjectionExtent, float Timeout, FString& OutError);
	bool ShouldConvert(const AEnemyCharacter& Enemy) const;
	void RegisterEnemy(AEnemyCharacter& Enemy);
	void UnregisterEnemy(AEnemyCharacter& Enemy);
	void RetryFailedConversions();
	bool IsEndingActive() const { return bEndingActive; }
	bool HasInitialCrowdFinished() const { return bInitialCrowdFinished; }
	int32 GetReadyCount() const;
	int32 GetPendingCount() const;
	const FString& GetLastError() const { return LastError; }

	// 최초 요청 당시 로드된 대상의 처리 완료만 알립니다. 이후 지역의 로딩을 기다리는 이벤트가 아닙니다.
	FSimpleMulticastDelegate OnInitialCrowdReady;
	FTinoEndingCrowdSessionError OnConversionError;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void OnWorldEndPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	enum class EConversionPhase : uint8 { WaitingForNavigation, Spawning, Ready, Failed };
	struct FConversion
	{
		TWeakObjectPtr<AEnemyCharacter> Enemy;
		TWeakObjectPtr<ATinoEndingCrowdSpawner> Spawner;
		EConversionPhase Phase = EConversionPhase::WaitingForNavigation;
		double PhaseStartTime = 0.0;
		bool bInitialPending = false;
		bool bHideWhileWaiting = true;
	};

	void ProcessConversion(FConversion& Conversion, int32& InFlightCount);
	void FailConversion(FConversion& Conversion, const FString& Reason);
	void ReleaseConversion(FConversion& Conversion);
	void Shutdown();

	// 약한 참조만 보관하여 몬스터의 셀을 강제로 로드 상태로 유지하지 않습니다.
	TMap<TWeakObjectPtr<AEnemyCharacter>, TSharedPtr<FConversion>> Conversions;
	UPROPERTY(Transient)
	TSoftObjectPtr<UMassEntityConfigAsset> EntityConfig;
	FName TargetTag;
	FVector NavProjectionExtent = FVector(50.0, 50.0, 200.0);
	float ConversionTimeout = 60.0f;
	float PollElapsed = 0.0f;
	int32 InitialPendingCount = 0;
	bool bEndingActive = false;
	bool bCollectingInitial = false;
	bool bInitialCrowdFinished = false;
	bool bShuttingDown = false;
	FString LastError;
};
