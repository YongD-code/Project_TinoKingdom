#include "TinoEndingCrowdSpawner.h"

#include "Engine/StreamableManager.h"
#include "TinoEndingCrowdSpawnPointsGenerator.h"

ATinoEndingCrowdSpawner::ATinoEndingCrowdSpawner()
{
	bAutoSpawnOnBeginPlay = false;
}

bool ATinoEndingCrowdSpawner::PrepareSpawnLocations(const TArray<FTransform>& Transforms)
{
	if (bAutoSpawnOnBeginPlay || Transforms.IsEmpty() || !PreparedTransforms.IsEmpty() || GetSpawnedEntityCount() != 0
		|| EntityTypes.Num() != 1 || EntityTypes[0].EntityConfig.IsNull()
		|| EntityTypes[0].Proportion <= 0.0f)
	{
		return false;
	}

	PreparedTransforms = Transforms;
	Count = Transforms.Num();
	SpawningCountScale = 1.0f;
	SpawnDataGenerators.Reset();
	FMassSpawnDataGenerator& Generator = SpawnDataGenerators.AddDefaulted_GetRef();
	Generator.GeneratorClass = UTinoEndingCrowdSpawnPointsGenerator::StaticClass();
	Generator.GeneratorInstance = NewObject<UTinoEndingCrowdSpawnPointsGenerator>(this);
	Generator.Proportion = 1.0f;
	return true;
}

void ATinoEndingCrowdSpawner::CancelEndingSpawn()
{
	// 비동기 로딩 완료 후 뒤늦게 생성되는 것을 막고, 이미 생성된 일부 군중도 정리합니다.
	PreparedTransforms.Reset();
	if (StreamingHandle.IsValid())
	{
		StreamingHandle->CancelHandle();
		StreamingHandle.Reset();
	}
	if (GetSpawnedEntityCount() > 0)
	{
		DoDespawning();
	}
}

int32 ATinoEndingCrowdSpawner::GetSpawnedEntityCount() const
{
	int32 Total = 0;
	for (const auto& Group : AllSpawnedEntities)
	{
		Total += Group.Entities.Num();
	}
	return Total;
}
