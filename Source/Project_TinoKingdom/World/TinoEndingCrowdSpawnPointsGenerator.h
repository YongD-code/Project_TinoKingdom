#pragma once

#include "CoreMinimal.h"
#include "MassEntitySpawnDataGeneratorBase.h"
#include "TinoEndingCrowdSpawnPointsGenerator.generated.h"

// 제어 액터가 미리 검증한 몬스터별 위치와 방향을 Mass 생성 데이터로 전달합니다.
UCLASS(meta = (DisplayName = "Tino Ending Crowd Spawn Points"))
class PROJECT_TINOKINGDOM_API UTinoEndingCrowdSpawnPointsGenerator : public UMassEntitySpawnDataGeneratorBase
{
	GENERATED_BODY()

public:
	virtual void Generate(UObject& QueryOwner, TConstArrayView<FMassSpawnedEntityType> EntityTypes,
		int32 Count, FFinishedGeneratingSpawnDataSignature& FinishedGeneratingSpawnPointsDelegate) const override;
};
