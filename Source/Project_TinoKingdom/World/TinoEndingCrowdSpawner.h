#pragma once

#include "CoreMinimal.h"
#include "MassSpawner.h"
#include "TinoEndingCrowdSpawner.generated.h"

// 몬스터별 위치에 정확히 한 명씩 생성하는 엔딩 전용 스포너입니다.
UCLASS(Blueprintable)
class PROJECT_TINOKINGDOM_API ATinoEndingCrowdSpawner : public AMassSpawner
{
	GENERATED_BODY()

public:
	ATinoEndingCrowdSpawner();

	// 엔티티 설정은 에디터에서 지정하고, 인원과 생성기는 전환 요청 시 구성합니다.
	bool PrepareSpawnLocations(const TArray<FTransform>& Transforms);
	void CancelEndingSpawn();
	int32 GetSpawnedEntityCount() const;
	TConstArrayView<FTransform> GetPreparedTransforms() const { return PreparedTransforms; }

private:
	UPROPERTY(Transient)
	TArray<FTransform> PreparedTransforms;
};
