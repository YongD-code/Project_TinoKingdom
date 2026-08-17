#pragma once

#include "Components/LineBatchComponent.h"
#include "Engine/World.h"

// DrawDebugHelpers는 Shipping 빌드에서 빈 함수가 되므로, 패키징 후에도 필요한
// 전투 판정 시각화는 런타임 LineBatchComponent를 직접 사용한다.
namespace TinoRuntimeDebugDraw
{
	inline ULineBatchComponent* GetLineBatcher(UWorld* World, float LifeTime)
	{
		if (World == nullptr)
		{
			return nullptr;
		}

		const UWorld::ELineBatcherType BatcherType = LifeTime > 0.f
			? UWorld::ELineBatcherType::WorldPersistent
			: UWorld::ELineBatcherType::World;

		return World->GetLineBatcher(BatcherType);
	}

	inline void DrawSweptSphere(
		UWorld* World,
		const FVector& Start,
		const FVector& End,
		float Radius,
		const FColor& Color,
		float LifeTime,
		int32 Segments = 16,
		float Thickness = 1.f)
	{
		ULineBatchComponent* LineBatcher = GetLineBatcher(World, LifeTime);
		if (LineBatcher == nullptr)
		{
			return;
		}

		const FVector SweepDelta = End - Start;
		const float SweepDistance = SweepDelta.Size();
		if (SweepDelta.IsNearlyZero())
		{
			LineBatcher->DrawSphere(
				Start,
				Radius,
				Segments,
				FLinearColor(Color),
				LifeTime,
				0,
				Thickness
			);
			return;
		}

		const FVector Center = (Start + End) * 0.5f;
		const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, SweepDelta / SweepDistance);

		// 구를 Start에서 End까지 Sweep한 부피는 같은 반지름의 캡슐과 같다.
		LineBatcher->DrawCapsule(
			Center,
			SweepDistance * 0.5f + Radius,
			Radius,
			Rotation,
			FLinearColor(Color),
			LifeTime,
			0,
			Thickness
		);
	}
}
