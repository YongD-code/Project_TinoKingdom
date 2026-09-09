#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "TinoEndingCrowdController.generated.h"

class AMassSpawner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTinoEndingCrowdSpawningFinishedSignature);

// 엔딩용 군중 스포너 하나를 제어합니다. 몬스터와 생성 위치는 변경하지 않습니다.
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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 기존 MetaHuman Mass Spawner를 지정합니다. 해당 스포너의 Auto Spawn on Begin Play는 꺼둡니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending Crowd")
	TObjectPtr<AMassSpawner> CrowdSpawner;

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

private:
	UFUNCTION()
	void HandleSpawningFinished();

	void UnbindSpawner();

	// 에디터 플레이 중 참조가 변경되어도 실제 생성 요청을 보낸 스포너를 추적합니다.
	UPROPERTY(Transient)
	TWeakObjectPtr<AMassSpawner> RequestedSpawner;

	FTimerHandle TestSpawnTimerHandle;
};
