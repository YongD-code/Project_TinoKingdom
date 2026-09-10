#include "TinoEndingCrowdSpawnPointsGenerator.h"

#include "MassSpawnLocationProcessor.h"
#include "TinoEndingCrowdSpawner.h"

void UTinoEndingCrowdSpawnPointsGenerator::Generate(UObject& QueryOwner,
	TConstArrayView<FMassSpawnedEntityType> EntityTypes, int32 Count,
	FFinishedGeneratingSpawnDataSignature& FinishedGeneratingSpawnPointsDelegate) const
{
	TArray<FMassEntitySpawnDataGeneratorResult> Results;
	const ATinoEndingCrowdSpawner* Spawner = Cast<ATinoEndingCrowdSpawner>(&QueryOwner);
	if (Spawner && EntityTypes.Num() == 1 && !Spawner->GetPreparedTransforms().IsEmpty())
	{
		const TConstArrayView<FTransform> Transforms = Spawner->GetPreparedTransforms();
		FMassEntitySpawnDataGeneratorResult& Result = Results.AddDefaulted_GetRef();
		Result.EntityConfigIndex = 0;
		// 엔딩 전환은 밀도 배율에 따른 Count 대신 대상 수를 사용해야 일대일 대응이 유지됩니다.
		Result.NumEntities = Transforms.Num();
		Result.SpawnDataProcessor = UMassSpawnLocationProcessor::StaticClass();
		Result.SpawnData.InitializeAs<FMassTransformsSpawnData>();
		FMassTransformsSpawnData& Data = Result.SpawnData.GetMutable<FMassTransformsSpawnData>();
		Data.Transforms.Append(Transforms.GetData(), Transforms.Num());
		Data.bRandomize = false;
	}
	FinishedGeneratingSpawnPointsDelegate.ExecuteIfBound(Results);
}
