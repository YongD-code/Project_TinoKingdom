#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "TinoEndingCrowdController.generated.h"

class AMassSpawner;
class AEnemyCharacter;
class ATinoEndingCrowdSpawner;
class UTinoEndingCrowdSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTinoEndingCrowdSpawningFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTinoEndingCrowdSpawnFailedSignature, FString, Reason);

// 엔딩용 군중 생성과 태그가 붙은 몬스터의 사람 전환을 제어합니다.
UCLASS(Blueprintable)
class PROJECT_TINOKINGDOM_API ATinoEndingCrowdController : public AActor
{
	GENERATED_BODY()

public:
	ATinoEndingCrowdController();

	// 일반 생성 또는 월드 전체의 엔딩 상태를 한 번 활성화합니다.
	UFUNCTION(BlueprintCallable, Category = "Ending Crowd")
	void SpawnEndingCrowd();

	// NavMesh 또는 에셋 문제를 해결한 뒤 실패한 대상만 다시 시도합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ending Crowd")
	void RetryFailedCrowdConversions();

	UFUNCTION(BlueprintPure, Category = "Ending Crowd|Status")
	int32 GetLoadedCrowdCount() const;

	UFUNCTION(BlueprintPure, Category = "Ending Crowd|Status")
	int32 GetPendingCrowdCount() const;

	// Mass 생성 작업의 완료를 알립니다. 렌더링이나 애니메이션 준비 완료를 의미하지는 않습니다.
	UPROPERTY(BlueprintAssignable, Category = "Ending Crowd")
	FTinoEndingCrowdSpawningFinishedSignature OnCrowdSpawningFinished;

	// 엔딩 활성화 당시 로드된 대상의 처리 완료입니다. 이후 스트리밍 대상은 별도로 계속 처리합니다.
	UPROPERTY(BlueprintAssignable, Category = "Ending Crowd")
	FTinoEndingCrowdSpawningFinishedSignature OnMonstersTransformed;

	UPROPERTY(BlueprintAssignable, Category = "Ending Crowd")
	FTinoEndingCrowdSpawnFailedSignature OnCrowdSpawnFailed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 몬스터 전환에서는 TinoEndingCrowdSpawner의 Entity Config만 읽습니다.
	// 스포너의 Auto Spawn on Begin Play는 꺼둡니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd")
	TObjectPtr<AMassSpawner> CrowdSpawner;

	// 켜면 엔딩 상태를 유지하며, 이후 로드되는 태그 대상도 발밑 위치에 한 명씩 생성합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Transformation")
	bool bTransformTaggedMonsters = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Transformation",
		meta = (EditCondition = "bTransformTaggedMonsters"))
	FName TargetMonsterTag = TEXT("EndingCrowdTarget");

	// 발밑에서 유효한 NavMesh를 찾는 각 축의 반경입니다. 단위는 센티미터입니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Transformation",
		meta = (EditCondition = "bTransformTaggedMonsters", ClampMin = "1.0", Units = "cm"))
	FVector NavProjectionExtent = FVector(50.0, 50.0, 200.0);

	// 각 대상의 NavMesh 준비와 군중 생성에 각각 적용하는 제한 시간입니다. 실패해도 전투 차단은 유지합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Transformation",
		meta = (EditCondition = "bTransformTaggedMonsters", ClampMin = "1.0", Units = "s"))
	float TransformationTimeout = 60.0f;

	// 임시 생성 테스트 옵션입니다. 엔딩 시퀀스에서 생성을 호출할 때는 꺼둡니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Testing")
	bool bTestSpawnOnBeginPlay = false;

	// 테스트 생성 요청까지 기다릴 게임 시간입니다. 단위는 초이며, 0이면 다음 틱에 요청합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Testing",
		meta = (EditCondition = "bTestSpawnOnBeginPlay", ClampMin = "0.0", Units = "s"))
	float TestSpawnDelay = 5.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ending Crowd|Status")
	bool bSpawnRequested = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ending Crowd|Status")
	bool bSpawningFinished = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ending Crowd|Status")
	bool bMonstersTransformed = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ending Crowd|Status")
	FString LastSpawnError;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ending Crowd|Status")
	bool bEndingWorldActive = false;

private:
	UFUNCTION()
	void HandleSpawningFinished();

	void HandleInitialCrowdReady();
	void HandleSessionError(const FString& Reason);
	void FailSpawn(const FString& Reason);
	void UnbindSpawner();
	void BindSession(UTinoEndingCrowdSubsystem& Session);

	// 에디터 플레이 중 참조가 변경되어도 실제 생성 요청을 보낸 스포너를 추적합니다.
	UPROPERTY(Transient)
	TWeakObjectPtr<AMassSpawner> RequestedSpawner;
	UPROPERTY(Transient)
	TWeakObjectPtr<UTinoEndingCrowdSubsystem> EndingSession;

	FTimerHandle TestSpawnTimerHandle;
};
