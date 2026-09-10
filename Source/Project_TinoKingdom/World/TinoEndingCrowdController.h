#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "TinoEndingCrowdController.generated.h"

class AMassSpawner;
class AEnemyCharacter;
class ATinoEndingCrowdSpawner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTinoEndingCrowdSpawningFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTinoEndingCrowdSpawnFailedSignature, FString, Reason);

// 엔딩용 군중 생성과 태그가 붙은 몬스터의 사람 전환을 제어합니다.
UCLASS(Blueprintable)
class PROJECT_TINOKINGDOM_API ATinoEndingCrowdController : public AActor
{
	GENERATED_BODY()

public:
	ATinoEndingCrowdController();

	// 제어 액터의 수명 동안 한 번만 생성을 요청합니다. 플레이 중 엔딩 이벤트에서 호출합니다.
	UFUNCTION(BlueprintCallable, Category = "Ending Crowd")
	void SpawnEndingCrowd();

	// Mass 생성 작업의 완료를 알립니다. 렌더링이나 애니메이션 준비 완료를 의미하지는 않습니다.
	UPROPERTY(BlueprintAssignable, Category = "Ending Crowd")
	FTinoEndingCrowdSpawningFinishedSignature OnCrowdSpawningFinished;

	// 생성한 군중을 확인하고 몬스터의 표시와 충돌을 끈 뒤 호출합니다.
	UPROPERTY(BlueprintAssignable, Category = "Ending Crowd")
	FTinoEndingCrowdSpawningFinishedSignature OnMonstersTransformed;

	UPROPERTY(BlueprintAssignable, Category = "Ending Crowd")
	FTinoEndingCrowdSpawnFailedSignature OnCrowdSpawnFailed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 일반 생성에는 기존 스포너, 몬스터 전환에는 TinoEndingCrowdSpawner를 지정합니다.
	// 스포너의 Auto Spawn on Begin Play는 꺼둡니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd")
	TObjectPtr<AMassSpawner> CrowdSpawner;

	// 켜면 격자 대신 태그가 붙은 살아 있는 몬스터의 발밑 위치에 한 명씩 생성합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Transformation")
	bool bTransformTaggedMonsters = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Transformation",
		meta = (EditCondition = "bTransformTaggedMonsters"))
	FName TargetMonsterTag = TEXT("EndingCrowdTarget");

	// 발밑에서 유효한 NavMesh를 찾는 각 축의 반경입니다. 단위는 센티미터입니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd|Transformation",
		meta = (EditCondition = "bTransformTaggedMonsters", ClampMin = "1.0", Units = "cm"))
	FVector NavProjectionExtent = FVector(50.0, 50.0, 200.0);

	// 전환 중 생성 완료를 기다릴 게임 시간입니다. 초과하면 생성 취소와 AI 복원을 수행합니다.
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

private:
	UFUNCTION()
	void HandleSpawningFinished();

	bool PrepareMonsterTransformation(ATinoEndingCrowdSpawner& Spawner);
	void FinalizeMonsterTransformation();
	void HandleTransformationTimeout();
	void FailSpawn(const FString& Reason);
	void RestorePendingMonsters();
	void UnbindSpawner();

	// 생성 완료 전까지 숨기지 않고 보존할 몬스터와 원래 상태입니다.
	struct FPendingMonster
	{
		TWeakObjectPtr<AEnemyCharacter> Enemy;
		bool bWasAIBlocked = false;
		bool bCouldBeDamaged = true;
	};
	TArray<FPendingMonster> PendingMonsters;
	bool bTransformationInProgress = false;

	// 에디터 플레이 중 참조가 변경되어도 실제 생성 요청을 보낸 스포너를 추적합니다.
	UPROPERTY(Transient)
	TWeakObjectPtr<AMassSpawner> RequestedSpawner;

	FTimerHandle TestSpawnTimerHandle;
	FTimerHandle TransformationTimeoutHandle;
	FTimerHandle FinalizeTransformationHandle;
};
