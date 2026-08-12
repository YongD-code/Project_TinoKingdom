// Copyright Epic Games, Inc. All Rights Reserved.

#include "Project_TinoKingdom.h"

#if WITH_EDITOR

#include "Editor.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeEditLayer.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "ScopedTransaction.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"
#include "WorldPartition/WorldPartitionHelpers.h"

namespace TinoLandscapeBiomePainter
{
	struct FInclusiveRect
	{
		int32 MinX = 0;
		int32 MinY = 0;
		int32 MaxX = -1;
		int32 MaxY = -1;

		bool IsValid() const { return MinX <= MaxX && MinY <= MaxY; }
		int32 Width() const { return MaxX - MinX + 1; }
		int32 Height() const { return MaxY - MinY + 1; }
		bool Contains(const int32 X, const int32 Y) const
		{
			return X >= MinX && X <= MaxX && Y >= MinY && Y <= MaxY;
		}
	};

	static void QuantizeWeights(const double InWeights[4], uint8 OutWeights[4])
	{
		double Fractions[4];
		int32 Sum = 0;
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const double Scaled = FMath::Clamp(InWeights[Index], 0.0, 1.0) * 255.0;
			OutWeights[Index] = static_cast<uint8>(FMath::FloorToInt(Scaled));
			Fractions[Index] = Scaled - static_cast<double>(OutWeights[Index]);
			Sum += OutWeights[Index];
		}

		for (int32 Remaining = 255 - Sum; Remaining > 0; --Remaining)
		{
			int32 BestIndex = 0;
			for (int32 Index = 1; Index < 4; ++Index)
			{
				if (Fractions[Index] > Fractions[BestIndex])
				{
					BestIndex = Index;
				}
			}
			++OutWeights[BestIndex];
			Fractions[BestIndex] = -1.0;
		}
	}

	static void ReadLayerRect(
		FLandscapeEditDataInterface& EditData,
		ULandscapeLayerInfoObject* LayerInfo,
		const FInclusiveRect& Rect,
		TArray<uint8>& OutData)
	{
		OutData.SetNumZeroed(Rect.Width() * Rect.Height());
		EditData.GetWeightDataFast(
			LayerInfo,
			Rect.MinX,
			Rect.MinY,
			Rect.MaxX,
			Rect.MaxY,
			OutData.GetData(),
			Rect.Width());
	}

	static void LogLayerSamples(
		ULandscapeInfo* LandscapeInfo,
		ULandscapeLayerInfoObject* const BiomeLayers[4],
		const FInclusiveRect& LandscapeRect,
		const FInclusiveRect& ProtectedRect,
		const TCHAR* Phase)
	{
		const int32 CenterX = FMath::RoundToInt((ProtectedRect.MinX + ProtectedRect.MaxX) * 0.5);
		const int32 CenterY = FMath::RoundToInt((ProtectedRect.MinY + ProtectedRect.MaxY) * 0.5);
		const FIntPoint Samples[4] =
		{
			FIntPoint(FMath::RoundToInt(FMath::Lerp(static_cast<double>(ProtectedRect.MaxX), static_cast<double>(LandscapeRect.MaxX), 0.75)), CenterY),
			FIntPoint(CenterX, FMath::RoundToInt(FMath::Lerp(static_cast<double>(ProtectedRect.MaxY), static_cast<double>(LandscapeRect.MaxY), 0.75))),
			FIntPoint(FMath::RoundToInt(FMath::Lerp(static_cast<double>(ProtectedRect.MinX), static_cast<double>(LandscapeRect.MinX), 0.75)), CenterY),
			FIntPoint(CenterX, FMath::RoundToInt(FMath::Lerp(static_cast<double>(ProtectedRect.MinY), static_cast<double>(LandscapeRect.MinY), 0.75)))
		};

		FLandscapeEditDataInterface ReadData(LandscapeInfo, false);
		ReadData.SetShouldDirtyPackage(false);
		for (int32 SampleIndex = 0; SampleIndex < 4; ++SampleIndex)
		{
			uint8 Weights[4] = { 0, 0, 0, 0 };
			for (int32 LayerIndex = 0; LayerIndex < 4; ++LayerIndex)
			{
				ReadData.GetWeightDataFast(
					BiomeLayers[LayerIndex],
					Samples[SampleIndex].X,
					Samples[SampleIndex].Y,
					Samples[SampleIndex].X,
					Samples[SampleIndex].Y,
					&Weights[LayerIndex],
					1);
			}

			UE_LOG(LogTemp, Display,
				TEXT("[TinoBiome] %s sample %d at (%d,%d)%s: grass=%u snow=%u mossyGrass=%u desertGround=%u"),
				Phase,
				SampleIndex,
				Samples[SampleIndex].X,
				Samples[SampleIndex].Y,
				ProtectedRect.Contains(Samples[SampleIndex].X, Samples[SampleIndex].Y) ? TEXT(" [castle-protected]") : TEXT(""),
				Weights[0],
				Weights[1],
				Weights[2],
				Weights[3]);
		}
	}

	static void PaintRect(
		FLandscapeEditDataInterface& EditData,
		ULandscapeInfo* LandscapeInfo,
		const FInclusiveRect& Rect,
		const FInclusiveRect& LandscapeRect,
		const FInclusiveRect& ProtectedRect,
		const TArray<ULandscapeLayerInfoObject*>& AllPaintLayers,
		ULandscapeLayerInfoObject* const BiomeLayers[4])
	{
		if (!Rect.IsValid())
		{
			return;
		}

		const int32 VertexCount = Rect.Width() * Rect.Height();
		const int32 LayerCount = LandscapeInfo->Layers.Num();
		TArray<uint8> PackedLayerData;
		PackedLayerData.SetNumZeroed(VertexCount * LayerCount);
		int32 BiomeLayerIndices[4];
		for (int32 LayerIndex = 0; LayerIndex < 4; ++LayerIndex)
		{
			BiomeLayerIndices[LayerIndex] = LandscapeInfo->GetLayerInfoIndex(BiomeLayers[LayerIndex]);
			check(BiomeLayerIndices[LayerIndex] != INDEX_NONE);
		}

		// Scores are normalized distances from the four sides of the protected castle
		// rectangle toward the matching Landscape edge. Equal adjacent scores form
		// diagonals that join each outer corner to the corresponding castle corner.
		const double RightSpan = FMath::Max(1, LandscapeRect.MaxX - ProtectedRect.MaxX);
		const double TopSpan = FMath::Max(1, LandscapeRect.MaxY - ProtectedRect.MaxY);
		const double LeftSpan = FMath::Max(1, ProtectedRect.MinX - LandscapeRect.MinX);
		const double BottomSpan = FMath::Max(1, ProtectedRect.MinY - LandscapeRect.MinY);
		const double LandscapeWidth = FMath::Max(1, LandscapeRect.MaxX - LandscapeRect.MinX);
		const double LandscapeHeight = FMath::Max(1, LandscapeRect.MaxY - LandscapeRect.MinY);
		const double BlendSoftness = 0.055;
		const double NoiseAmplitude = 0.045;
		const double TwoPi = 2.0 * UE_DOUBLE_PI;

		for (int32 Y = Rect.MinY; Y <= Rect.MaxY; ++Y)
		{
			for (int32 X = Rect.MinX; X <= Rect.MaxX; ++X)
			{
				const double NormalizedX = (X - LandscapeRect.MinX) / LandscapeWidth;
				const double NormalizedY = (Y - LandscapeRect.MinY) / LandscapeHeight;
				double Scores[4] =
				{
					(X - ProtectedRect.MaxX) / RightSpan, // Right: grass
					(Y - ProtectedRect.MaxY) / TopSpan,   // Top (+Y): snow
					(ProtectedRect.MinX - X) / LeftSpan,  // Left: MossyGrass
					(ProtectedRect.MinY - Y) / BottomSpan // Bottom (-Y): desertGround
				};

				double RadialProgress = Scores[0];
				for (int32 LayerIndex = 1; LayerIndex < 4; ++LayerIndex)
				{
					RadialProgress = FMath::Max(RadialProgress, Scores[LayerIndex]);
				}
				RadialProgress = FMath::Clamp(RadialProgress, 0.0, 1.0);
				const double WarpEnvelope = 4.0 * RadialProgress * (1.0 - RadialProgress);
				Scores[0] += NoiseAmplitude * WarpEnvelope *
					(0.72 * FMath::Sin(TwoPi * NormalizedY * 0.83 + 0.35) +
					 0.28 * FMath::Sin(TwoPi * (NormalizedX + NormalizedY * 0.31) * 0.57 + 1.80));
				Scores[1] += NoiseAmplitude * WarpEnvelope *
					(0.70 * FMath::Sin(TwoPi * NormalizedX * 0.79 + 2.10) +
					 0.30 * FMath::Sin(TwoPi * (NormalizedY - NormalizedX * 0.27) * 0.52 + 0.65));
				Scores[2] += NoiseAmplitude * WarpEnvelope *
					(0.68 * FMath::Sin(TwoPi * NormalizedY * 0.74 + 3.05) +
					 0.32 * FMath::Sin(TwoPi * (NormalizedX - NormalizedY * 0.24) * 0.61 + 2.35));
				Scores[3] += NoiseAmplitude * WarpEnvelope *
					(0.73 * FMath::Sin(TwoPi * NormalizedX * 0.86 + 4.15) +
					 0.27 * FMath::Sin(TwoPi * (NormalizedY + NormalizedX * 0.29) * 0.55 + 3.25));
				double MaxScore = Scores[0];
				for (int32 LayerIndex = 1; LayerIndex < 4; ++LayerIndex)
				{
					MaxScore = FMath::Max(MaxScore, Scores[LayerIndex]);
				}

				double Weights[4];
				double WeightSum = 0.0;
				for (int32 LayerIndex = 0; LayerIndex < 4; ++LayerIndex)
				{
					Weights[LayerIndex] = FMath::Exp((Scores[LayerIndex] - MaxScore) / BlendSoftness);
					WeightSum += Weights[LayerIndex];
				}
				for (double& Weight : Weights)
				{
					Weight /= WeightSum;
				}

				uint8 Quantized[4];
				QuantizeWeights(Weights, Quantized);
				const int32 DataIndex = (X - Rect.MinX) + (Y - Rect.MinY) * Rect.Width();
				for (int32 LayerIndex = 0; LayerIndex < 4; ++LayerIndex)
				{
					PackedLayerData[DataIndex * LayerCount + BiomeLayerIndices[LayerIndex]] = Quantized[LayerIndex];
				}
			}
		}

		TSet<ULandscapeLayerInfoObject*> DirtyLayers;
		for (ULandscapeLayerInfoObject* LayerInfo : AllPaintLayers)
		{
			DirtyLayers.Add(LayerInfo);
		}
		EditData.SetAlphaData(
			DirtyLayers,
			Rect.MinX,
			Rect.MinY,
			Rect.MaxX,
			Rect.MaxY,
			PackedLayerData.GetData(),
			Rect.Width() * LayerCount,
			ELandscapeLayerPaintingRestriction::None);
	}

	static void PaintLandscapeBiomes(const TArray<FString>& Args)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || World->WorldType != EWorldType::Editor)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiome] No editor world is open."));
			return;
		}

		ALandscape* Landscape = nullptr;
		for (TActorIterator<ALandscape> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				if (Landscape)
				{
					UE_LOG(LogTemp, Error, TEXT("[TinoBiome] More than one root Landscape actor is loaded; aborting."));
					return;
				}
				Landscape = *It;
			}
		}

		if (!Landscape)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiome] No root Landscape actor was found."));
			return;
		}

		ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
		int32 MinX = 0;
		int32 MinY = 0;
		int32 MaxX = 0;
		int32 MaxY = 0;
		if (!LandscapeInfo || !LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiome] Landscape extent is unavailable."));
			return;
		}

		if (!Landscape->GetEditingLayer().IsValid())
		{
			ULandscapeEditLayerBase* SelectedEditLayer = nullptr;
			for (ULandscapeEditLayerBase* EditLayer : Landscape->GetEditLayers())
			{
				UE_LOG(LogTemp, Display, TEXT("[TinoBiome] EditLayer=%s Guid=%s Locked=%s SupportsEditingTools=%s"),
					*EditLayer->GetName().ToString(),
					*EditLayer->GetGuid().ToString(),
					EditLayer->IsLocked() ? TEXT("true") : TEXT("false"),
					EditLayer->SupportsEditingTools() ? TEXT("true") : TEXT("false"));
				if (!SelectedEditLayer && !EditLayer->IsLocked() && EditLayer->SupportsEditingTools())
				{
					SelectedEditLayer = EditLayer;
				}
			}

			if (!SelectedEditLayer)
			{
				UE_LOG(LogTemp, Error, TEXT("[TinoBiome] No unlocked Landscape Edit Layer supports paint tools; aborting safely."));
				return;
			}

			Landscape->SetEditingLayer(SelectedEditLayer->GetGuid());
			UE_LOG(LogTemp, Display, TEXT("[TinoBiome] Selected current unlocked paint edit layer '%s' (%s)."),
				*SelectedEditLayer->GetName().ToString(), *SelectedEditLayer->GetGuid().ToString());
		}

		const FInclusiveRect LandscapeRect { MinX, MinY, MaxX, MaxY };
		const FTransform LandscapeToWorld = Landscape->LandscapeActorToWorld();
		FBox LandscapeWorldBounds(ForceInit);
		for (const FVector& Corner :
			{ FVector(MinX, MinY, 0.0), FVector(MaxX, MinY, 0.0), FVector(MinX, MaxY, 0.0), FVector(MaxX, MaxY, 0.0) })
		{
			LandscapeWorldBounds += LandscapeToWorld.TransformPosition(Corner);
		}

		UE_LOG(LogTemp, Display,
			TEXT("[TinoBiome] Level=%s Landscape=%s Extent=(%d,%d)-(%d,%d) WorldXY=(%.1f,%.1f)-(%.1f,%.1f) Material=%s EditingLayer=%s"),
			*World->GetPackage()->GetName(),
			*Landscape->GetActorLabel(),
			MinX, MinY, MaxX, MaxY,
			LandscapeWorldBounds.Min.X, LandscapeWorldBounds.Min.Y,
			LandscapeWorldBounds.Max.X, LandscapeWorldBounds.Max.Y,
			*GetPathNameSafe(Landscape->GetLandscapeMaterial()),
			*Landscape->GetEditingLayer().ToString());

		TArray<ULandscapeLayerInfoObject*> AllPaintLayers;
		for (const FLandscapeInfoLayerSettings& LayerSettings : LandscapeInfo->Layers)
		{
			ULandscapeLayerInfoObject* LayerInfo = LayerSettings.LayerInfoObj;
			UE_LOG(LogTemp, Display, TEXT("[TinoBiome] TargetLayer=%s LayerInfo=%s BlendMethod=%d"),
				*LayerSettings.GetLayerName().ToString(),
				*GetPathNameSafe(LayerInfo),
				LayerInfo ? static_cast<int32>(LayerInfo->GetBlendMethod()) : -1);
			if (LayerInfo && LayerInfo != ALandscape::VisibilityLayer)
			{
				AllPaintLayers.AddUnique(LayerInfo);
			}
		}

		const FName BiomeNames[4] =
		{
			TEXT("grass"),
			TEXT("snow"),
			TEXT("mossyGrass"),
			TEXT("desertGround")
		};
		ULandscapeLayerInfoObject* BiomeLayers[4] = { nullptr, nullptr, nullptr, nullptr };
		for (int32 LayerIndex = 0; LayerIndex < 4; ++LayerIndex)
		{
			BiomeLayers[LayerIndex] = LandscapeInfo->GetLayerInfoByName(BiomeNames[LayerIndex], Landscape);
			if (!BiomeLayers[LayerIndex])
			{
				UE_LOG(LogTemp, Error, TEXT("[TinoBiome] Required Target Layer '%s' has no active LayerInfo; aborting."), *BiomeNames[LayerIndex].ToString());
				return;
			}
			if (BiomeLayers[LayerIndex]->GetBlendMethod() != ELandscapeTargetLayerBlendMethod::FinalWeightBlending)
			{
				UE_LOG(LogTemp, Error, TEXT("[TinoBiome] Target Layer '%s' is not Weight Blended; aborting."), *BiomeNames[LayerIndex].ToString());
				return;
			}
		}

		FBox CastleWorldBounds(ForceInit);
		int32 CastleActorCount = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			AStaticMeshActor* Actor = *It;
			if (!Actor->GetActorLabel().Contains(TEXT("castle"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			const FBox ActorBounds = Actor->GetComponentsBoundingBox(true);
			if (ActorBounds.IsValid)
			{
				CastleWorldBounds += ActorBounds;
				++CastleActorCount;
				UE_LOG(LogTemp, Display, TEXT("[TinoBiome] CastleActor=%s Bounds=(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)"),
					*Actor->GetActorLabel(),
					ActorBounds.Min.X, ActorBounds.Min.Y, ActorBounds.Min.Z,
					ActorBounds.Max.X, ActorBounds.Max.Y, ActorBounds.Max.Z);
			}
		}

		// World Partition may keep the castle actors unloaded during editor startup.
		// Actor descriptors still provide their saved labels and editor bounds without
		// loading or modifying those actors.
		if (CastleActorCount == 0)
		{
			if (UWorldPartition* WorldPartition = World->GetWorldPartition())
			{
				FWorldPartitionHelpers::ForEachActorDescInstance<AStaticMeshActor>(
					WorldPartition,
					[&CastleWorldBounds, &CastleActorCount](const FWorldPartitionActorDescInstance* ActorDescInstance)
					{
						if (ActorDescInstance && ActorDescInstance->GetActorLabelString().Contains(TEXT("castle"), ESearchCase::IgnoreCase))
						{
							const FBox ActorBounds = ActorDescInstance->GetEditorBounds();
							if (ActorBounds.IsValid)
							{
								CastleWorldBounds += ActorBounds;
								++CastleActorCount;
							}
						}
						return true;
					});
			}
		}

		if (CastleActorCount == 0 || !CastleWorldBounds.IsValid)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiome] No valid Static Mesh actor containing 'castle' was found; aborting to protect the castle paint."));
			return;
		}

		constexpr double CastleSafetyMargin = 300.0;
		CastleWorldBounds = CastleWorldBounds.ExpandBy(FVector(CastleSafetyMargin, CastleSafetyMargin, 0.0));
		FBox CastleLocalBounds(ForceInit);
		const FTransform WorldToLandscape = LandscapeToWorld.Inverse();
		for (const FVector& Corner :
			{ FVector(CastleWorldBounds.Min.X, CastleWorldBounds.Min.Y, 0.0),
			  FVector(CastleWorldBounds.Max.X, CastleWorldBounds.Min.Y, 0.0),
			  FVector(CastleWorldBounds.Min.X, CastleWorldBounds.Max.Y, 0.0),
			  FVector(CastleWorldBounds.Max.X, CastleWorldBounds.Max.Y, 0.0) })
		{
			CastleLocalBounds += WorldToLandscape.TransformPosition(Corner);
		}

		FInclusiveRect ProtectedRect
		{
			FMath::Clamp(FMath::FloorToInt(CastleLocalBounds.Min.X), MinX, MaxX),
			FMath::Clamp(FMath::FloorToInt(CastleLocalBounds.Min.Y), MinY, MaxY),
			FMath::Clamp(FMath::CeilToInt(CastleLocalBounds.Max.X), MinX, MaxX),
			FMath::Clamp(FMath::CeilToInt(CastleLocalBounds.Max.Y), MinY, MaxY)
		};

		UE_LOG(LogTemp, Display,
			TEXT("[TinoBiome] Castle union: %d actors, WorldXY=(%.1f,%.1f)-(%.1f,%.1f), protected landscape rect=(%d,%d)-(%d,%d) including %.0f uu margin."),
			CastleActorCount,
			CastleWorldBounds.Min.X, CastleWorldBounds.Min.Y,
			CastleWorldBounds.Max.X, CastleWorldBounds.Max.Y,
			ProtectedRect.MinX, ProtectedRect.MinY, ProtectedRect.MaxX, ProtectedRect.MaxY,
			CastleSafetyMargin);

		TMap<ULandscapeLayerInfoObject*, TArray<uint8>> ProtectedWeightsBefore;
		{
			FLandscapeEditDataInterface ReadData(LandscapeInfo, false);
			ReadData.SetShouldDirtyPackage(false);
			for (ULandscapeLayerInfoObject* LayerInfo : AllPaintLayers)
			{
				ReadLayerRect(ReadData, LayerInfo, ProtectedRect, ProtectedWeightsBefore.Add(LayerInfo));
			}
		}
		LogLayerSamples(LandscapeInfo, BiomeLayers, LandscapeRect, ProtectedRect, TEXT("BEFORE"));

		const FInclusiveRect ExteriorRects[4] =
		{
			{ MinX, MinY, MaxX, ProtectedRect.MinY - 1 },
			{ MinX, ProtectedRect.MaxY + 1, MaxX, MaxY },
			{ MinX, ProtectedRect.MinY, ProtectedRect.MinX - 1, ProtectedRect.MaxY },
			{ ProtectedRect.MaxX + 1, ProtectedRect.MinY, MaxX, ProtectedRect.MaxY }
		};

		{
			const FScopedTransaction Transaction(NSLOCTEXT("TinoKingdom", "PaintLandscapeBiomes", "Paint TinoKingdom Landscape Biomes"));
			Landscape->Modify();
			FLandscapeEditDataInterface EditData(LandscapeInfo);
			EditData.SetShouldDirtyPackage(true);
			for (const FInclusiveRect& ExteriorRect : ExteriorRects)
			{
				PaintRect(EditData, LandscapeInfo, ExteriorRect, LandscapeRect, ProtectedRect, AllPaintLayers, BiomeLayers);
			}
		}

		Landscape->RequestLayersContentUpdateForceAll(ELandscapeLayerUpdateMode::Update_Weightmap_All, true);
		LandscapeInfo->UpdateAllComponentMaterialInstances(/*bInInvalidateCombinationMaterials=*/true);

		bool bCastleUnchanged = true;
		{
			FLandscapeEditDataInterface ReadData(LandscapeInfo, false);
			ReadData.SetShouldDirtyPackage(false);
			for (ULandscapeLayerInfoObject* LayerInfo : AllPaintLayers)
			{
				TArray<uint8> After;
				ReadLayerRect(ReadData, LayerInfo, ProtectedRect, After);
				if (After != ProtectedWeightsBefore.FindChecked(LayerInfo))
				{
					bCastleUnchanged = false;
					UE_LOG(LogTemp, Error, TEXT("[TinoBiome] Castle protection verification failed for layer %s."), *LayerInfo->LayerName.ToString());
				}
			}
		}

		if (!bCastleUnchanged)
		{
			GEditor->UndoTransaction();
			UE_LOG(LogTemp, Error, TEXT("[TinoBiome] The entire paint operation was undone because castle weights changed."));
			return;
		}

		LogLayerSamples(LandscapeInfo, BiomeLayers, LandscapeRect, ProtectedRect, TEXT("AFTER"));
		Landscape->MarkPackageDirty();
		UE_LOG(LogTemp, Display,
			TEXT("[TinoBiome] SUCCESS CASTLE-CORNER SPLIT. Right=grass, Top(+Y)=snow, Left=mossyGrass, Bottom(-Y)=desertGround. Height, materials, LayerInfo assets, actor transforms, and castle-protected weights were not changed. The level remains dirty and unsaved."));
	}

	static FAutoConsoleCommand PaintLandscapeBiomesCommand(
		TEXT("TinoKingdom.PaintLandscapeBiomes"),
		TEXT("Repaint the Landscape exterior into four castle-corner trapezoid biomes while preserving castle weights."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&PaintLandscapeBiomes));

	static void TransferRockToMossyGrass(const TArray<FString>& Args)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || World->WorldType != EWorldType::Editor)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiomeTransfer] No editor world is open."));
			return;
		}

		ALandscape* Landscape = nullptr;
		for (TActorIterator<ALandscape> It(World); It; ++It)
		{
			if (!IsValid(*It))
			{
				continue;
			}
			if (Landscape)
			{
				UE_LOG(LogTemp, Error, TEXT("[TinoBiomeTransfer] More than one root Landscape is loaded; aborting."));
				return;
			}
			Landscape = *It;
		}

		ULandscapeInfo* LandscapeInfo = Landscape ? Landscape->GetLandscapeInfo() : nullptr;
		int32 MinX = 0;
		int32 MinY = 0;
		int32 MaxX = 0;
		int32 MaxY = 0;
		if (!Landscape || !LandscapeInfo || !LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiomeTransfer] Landscape extent is unavailable."));
			return;
		}

		ULandscapeLayerInfoObject* RockLayer = LandscapeInfo->GetLayerInfoByName(TEXT("rock"), Landscape);
		ULandscapeLayerInfoObject* MossyGrassLayer = LandscapeInfo->GetLayerInfoByName(TEXT("mossyGrass"), Landscape);
		if (!RockLayer || !MossyGrassLayer)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiomeTransfer] Rock or MossyGrass has no active LayerInfo; aborting."));
			return;
		}
		if (RockLayer->GetBlendMethod() != ELandscapeTargetLayerBlendMethod::FinalWeightBlending ||
			MossyGrassLayer->GetBlendMethod() != ELandscapeTargetLayerBlendMethod::FinalWeightBlending)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiomeTransfer] Rock and MossyGrass must both use Final Weight Blending."));
			return;
		}

		FBox CastleWorldBounds(ForceInit);
		int32 CastleActorCount = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			AStaticMeshActor* Actor = *It;
			if (!Actor->GetActorLabel().Contains(TEXT("castle"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			const FBox ActorBounds = Actor->GetComponentsBoundingBox(true);
			if (ActorBounds.IsValid)
			{
				CastleWorldBounds += ActorBounds;
				++CastleActorCount;
			}
		}

		if (CastleActorCount == 0 || !CastleWorldBounds.IsValid)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiomeTransfer] Castle bounds are unavailable; aborting safely."));
			return;
		}

		constexpr double CastleSafetyMargin = 300.0;
		CastleWorldBounds = CastleWorldBounds.ExpandBy(FVector(CastleSafetyMargin, CastleSafetyMargin, 0.0));
		const FTransform WorldToLandscape = Landscape->LandscapeActorToWorld().Inverse();
		FBox CastleLocalBounds(ForceInit);
		for (const FVector& Corner :
			{ FVector(CastleWorldBounds.Min.X, CastleWorldBounds.Min.Y, 0.0),
			  FVector(CastleWorldBounds.Max.X, CastleWorldBounds.Min.Y, 0.0),
			  FVector(CastleWorldBounds.Min.X, CastleWorldBounds.Max.Y, 0.0),
			  FVector(CastleWorldBounds.Max.X, CastleWorldBounds.Max.Y, 0.0) })
		{
			CastleLocalBounds += WorldToLandscape.TransformPosition(Corner);
		}

		const FInclusiveRect LandscapeRect { MinX, MinY, MaxX, MaxY };
		const FInclusiveRect ProtectedRect
		{
			FMath::Clamp(FMath::FloorToInt(CastleLocalBounds.Min.X), MinX, MaxX),
			FMath::Clamp(FMath::FloorToInt(CastleLocalBounds.Min.Y), MinY, MaxY),
			FMath::Clamp(FMath::CeilToInt(CastleLocalBounds.Max.X), MinX, MaxX),
			FMath::Clamp(FMath::CeilToInt(CastleLocalBounds.Max.Y), MinY, MaxY)
		};

		FGuid SourceEditLayerGuid;
		FName SourceEditLayerName = NAME_None;
		int64 LargestExteriorRockSum = 0;
		for (ULandscapeEditLayerBase* EditLayer : Landscape->GetEditLayers())
		{
			if (!EditLayer || EditLayer->IsLocked() || !EditLayer->SupportsEditingTools())
			{
				continue;
			}

			FLandscapeEditDataInterface ReadData(LandscapeInfo, false);
			ReadData.SetShouldDirtyPackage(false);
			ReadData.SetEditLayer(EditLayer->GetGuid());
			TArray<uint8> RockData;
			ReadLayerRect(ReadData, RockLayer, LandscapeRect, RockData);
			int64 ExteriorRockSum = 0;
			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				for (int32 X = MinX; X <= MaxX; ++X)
				{
					if (!ProtectedRect.Contains(X, Y))
					{
						ExteriorRockSum += RockData[(X - MinX) + (Y - MinY) * LandscapeRect.Width()];
					}
				}
			}

			UE_LOG(LogTemp, Display, TEXT("[TinoBiomeTransfer] EditLayer=%s exterior Rock sum=%lld"),
				*EditLayer->GetName().ToString(), ExteriorRockSum);
			if (ExteriorRockSum > LargestExteriorRockSum)
			{
				LargestExteriorRockSum = ExteriorRockSum;
				SourceEditLayerGuid = EditLayer->GetGuid();
				SourceEditLayerName = EditLayer->GetName();
			}
		}

		if (!SourceEditLayerGuid.IsValid() || LargestExteriorRockSum == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiomeTransfer] No unlocked editable layer contains exterior Rock weight."));
			return;
		}

		const int32 LayerCount = LandscapeInfo->Layers.Num();
		const int32 VertexCount = LandscapeRect.Width() * LandscapeRect.Height();
		const int32 RockLayerIndex = LandscapeInfo->GetLayerInfoIndex(RockLayer);
		const int32 MossyGrassLayerIndex = LandscapeInfo->GetLayerInfoIndex(MossyGrassLayer);
		if (RockLayerIndex == INDEX_NONE || MossyGrassLayerIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoBiomeTransfer] Layer indices are unavailable."));
			return;
		}

		TArray<TArray<uint8>> OriginalLayerData;
		OriginalLayerData.SetNum(LayerCount);
		{
			FLandscapeEditDataInterface ReadData(LandscapeInfo, false);
			ReadData.SetShouldDirtyPackage(false);
			ReadData.SetEditLayer(SourceEditLayerGuid);
			for (int32 LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
			{
				ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[LayerIndex].LayerInfoObj;
				if (LayerInfo)
				{
					ReadLayerRect(ReadData, LayerInfo, LandscapeRect, OriginalLayerData[LayerIndex]);
				}
				else
				{
					OriginalLayerData[LayerIndex].SetNumZeroed(VertexCount);
				}
			}
		}

		TArray<uint8> PackedLayerData;
		PackedLayerData.SetNumUninitialized(VertexCount * LayerCount);
		for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
		{
			for (int32 LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
			{
				PackedLayerData[VertexIndex * LayerCount + LayerIndex] = OriginalLayerData[LayerIndex][VertexIndex];
			}
		}

		int64 RockWeightTransferred = 0;
		int64 MossyGrassWeightBefore = 0;
		int32 ChangedVertexCount = 0;
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				if (ProtectedRect.Contains(X, Y))
				{
					continue;
				}
				const int32 VertexIndex = (X - MinX) + (Y - MinY) * LandscapeRect.Width();
				uint8& RockWeight = PackedLayerData[VertexIndex * LayerCount + RockLayerIndex];
				uint8& MossyGrassWeight = PackedLayerData[VertexIndex * LayerCount + MossyGrassLayerIndex];
				MossyGrassWeightBefore += MossyGrassWeight;
				if (RockWeight > 0)
				{
					RockWeightTransferred += RockWeight;
					MossyGrassWeight = static_cast<uint8>(FMath::Min(255, static_cast<int32>(MossyGrassWeight) + static_cast<int32>(RockWeight)));
					RockWeight = 0;
					++ChangedVertexCount;
				}
			}
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT("TinoKingdom", "TransferRockToMossyGrass", "Transfer Rock Landscape Weight to MossyGrass"));
			Landscape->Modify();
			FLandscapeEditDataInterface EditData(LandscapeInfo);
			EditData.SetShouldDirtyPackage(true);
			EditData.SetEditLayer(SourceEditLayerGuid);
			TSet<ULandscapeLayerInfoObject*> DirtyLayers { RockLayer, MossyGrassLayer };
			EditData.SetAlphaData(
				DirtyLayers,
				MinX, MinY, MaxX, MaxY,
				PackedLayerData.GetData(),
				LandscapeRect.Width() * LayerCount,
				ELandscapeLayerPaintingRestriction::None);
		}

		Landscape->RequestLayersContentUpdateForceAll(ELandscapeLayerUpdateMode::Update_Weightmap_All, true);
		LandscapeInfo->UpdateAllComponentMaterialInstances(/*bInInvalidateCombinationMaterials=*/true);

		bool bCastleUnchanged = true;
		int64 RockWeightAfter = 0;
		int64 MossyGrassWeightAfter = 0;
		{
			FLandscapeEditDataInterface ReadData(LandscapeInfo, false);
			ReadData.SetShouldDirtyPackage(false);
			ReadData.SetEditLayer(SourceEditLayerGuid);
			for (int32 LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
			{
				ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[LayerIndex].LayerInfoObj;
				if (!LayerInfo)
				{
					continue;
				}
				TArray<uint8> After;
				ReadLayerRect(ReadData, LayerInfo, LandscapeRect, After);
				for (int32 Y = MinY; Y <= MaxY; ++Y)
				{
					for (int32 X = MinX; X <= MaxX; ++X)
					{
						const int32 VertexIndex = (X - MinX) + (Y - MinY) * LandscapeRect.Width();
						if (ProtectedRect.Contains(X, Y) && After[VertexIndex] != OriginalLayerData[LayerIndex][VertexIndex])
						{
							bCastleUnchanged = false;
						}
						else if (!ProtectedRect.Contains(X, Y))
						{
							if (LayerIndex == RockLayerIndex)
							{
								RockWeightAfter += After[VertexIndex];
							}
							else if (LayerIndex == MossyGrassLayerIndex)
							{
								MossyGrassWeightAfter += After[VertexIndex];
							}
						}
					}
				}
			}
		}

		const int64 ExpectedMossyGrassWeight = MossyGrassWeightBefore + RockWeightTransferred;
		if (!bCastleUnchanged || RockWeightAfter != 0 || MossyGrassWeightAfter != ExpectedMossyGrassWeight)
		{
			GEditor->UndoTransaction();
			UE_LOG(LogTemp, Error,
				TEXT("[TinoBiomeTransfer] Verification failed; operation undone. CastleUnchanged=%s RockAfter=%lld MossAfter=%lld ExpectedMoss=%lld"),
				bCastleUnchanged ? TEXT("true") : TEXT("false"), RockWeightAfter, MossyGrassWeightAfter, ExpectedMossyGrassWeight);
			return;
		}

		Landscape->MarkPackageDirty();
		UE_LOG(LogTemp, Display,
			TEXT("[TinoBiomeTransfer] SUCCESS. EditLayer=%s ChangedVertices=%d TransferredWeight=%lld RockAfter=%lld MossAfter=%lld Castle weights unchanged. Height data unchanged."),
			*SourceEditLayerName.ToString(), ChangedVertexCount, RockWeightTransferred, RockWeightAfter, MossyGrassWeightAfter);
	}

	static FAutoConsoleCommand TransferRockToMossyGrassCommand(
		TEXT("TinoKingdom.TransferRockToMossyGrass"),
		TEXT("Transfer all exterior Rock paint weight to MossyGrass while preserving castle and all other layer weights."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&TransferRockToMossyGrass));

	static void BlendCastlePaintBoundary(const TArray<FString>& Args)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || World->WorldType != EWorldType::Editor)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoCastleBlend] No editor world is open."));
			return;
		}

		ALandscape* Landscape = nullptr;
		for (TActorIterator<ALandscape> It(World); It; ++It)
		{
			if (!IsValid(*It))
			{
				continue;
			}
			if (Landscape)
			{
				UE_LOG(LogTemp, Error, TEXT("[TinoCastleBlend] More than one root Landscape is loaded; aborting."));
				return;
			}
			Landscape = *It;
		}

		ULandscapeInfo* LandscapeInfo = Landscape ? Landscape->GetLandscapeInfo() : nullptr;
		int32 MinX = 0;
		int32 MinY = 0;
		int32 MaxX = 0;
		int32 MaxY = 0;
		if (!Landscape || !LandscapeInfo || !LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoCastleBlend] Landscape extent is unavailable."));
			return;
		}

		FBox CastleWorldBounds(ForceInit);
		int32 CastleActorCount = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			AStaticMeshActor* Actor = *It;
			if (!Actor->GetActorLabel().Contains(TEXT("castle"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			const FBox ActorBounds = Actor->GetComponentsBoundingBox(true);
			if (ActorBounds.IsValid)
			{
				CastleWorldBounds += ActorBounds;
				++CastleActorCount;
			}
		}

		if (CastleActorCount == 0)
		{
			if (UWorldPartition* WorldPartition = World->GetWorldPartition())
			{
				FWorldPartitionHelpers::ForEachActorDescInstance<AStaticMeshActor>(
					WorldPartition,
					[&CastleWorldBounds, &CastleActorCount](const FWorldPartitionActorDescInstance* ActorDescInstance)
					{
						if (ActorDescInstance && ActorDescInstance->GetActorLabelString().Contains(TEXT("castle"), ESearchCase::IgnoreCase))
						{
							const FBox ActorBounds = ActorDescInstance->GetEditorBounds();
							if (ActorBounds.IsValid)
							{
								CastleWorldBounds += ActorBounds;
								++CastleActorCount;
							}
						}
						return true;
					});
			}
		}

		if (CastleActorCount == 0 || !CastleWorldBounds.IsValid)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoCastleBlend] Castle bounds are unavailable from loaded actors and World Partition descriptors; aborting safely."));
			return;
		}

		constexpr double CastleSafetyMargin = 300.0;
		constexpr double BlendWidthWorld = 6000.0;
		CastleWorldBounds = CastleWorldBounds.ExpandBy(FVector(CastleSafetyMargin, CastleSafetyMargin, 0.0));
		const FTransform LandscapeToWorld = Landscape->LandscapeActorToWorld();
		const FTransform WorldToLandscape = LandscapeToWorld.Inverse();
		FBox CastleLocalBounds(ForceInit);
		for (const FVector& Corner :
			{ FVector(CastleWorldBounds.Min.X, CastleWorldBounds.Min.Y, 0.0),
			  FVector(CastleWorldBounds.Max.X, CastleWorldBounds.Min.Y, 0.0),
			  FVector(CastleWorldBounds.Min.X, CastleWorldBounds.Max.Y, 0.0),
			  FVector(CastleWorldBounds.Max.X, CastleWorldBounds.Max.Y, 0.0) })
		{
			CastleLocalBounds += WorldToLandscape.TransformPosition(Corner);
		}

		const FInclusiveRect LandscapeRect { MinX, MinY, MaxX, MaxY };
		const FInclusiveRect ProtectedRect
		{
			FMath::Clamp(FMath::FloorToInt(CastleLocalBounds.Min.X), MinX, MaxX),
			FMath::Clamp(FMath::FloorToInt(CastleLocalBounds.Min.Y), MinY, MaxY),
			FMath::Clamp(FMath::CeilToInt(CastleLocalBounds.Max.X), MinX, MaxX),
			FMath::Clamp(FMath::CeilToInt(CastleLocalBounds.Max.Y), MinY, MaxY)
		};

		const FName BiomeNames[] = { TEXT("grass"), TEXT("snow"), TEXT("mossyGrass"), TEXT("desertGround") };
		TArray<ULandscapeLayerInfoObject*> BiomeLayers;
		for (const FName BiomeName : BiomeNames)
		{
			ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->GetLayerInfoByName(BiomeName, Landscape);
			if (!LayerInfo || LayerInfo->GetBlendMethod() != ELandscapeTargetLayerBlendMethod::FinalWeightBlending)
			{
				UE_LOG(LogTemp, Error, TEXT("[TinoCastleBlend] Required Weight-Blended layer '%s' is unavailable."), *BiomeName.ToString());
				return;
			}
			BiomeLayers.Add(LayerInfo);
		}

		FGuid SourceEditLayerGuid;
		FName SourceEditLayerName;
		int64 LargestExteriorBiomeSum = 0;
		for (ULandscapeEditLayerBase* EditLayer : Landscape->GetEditLayers())
		{
			if (!EditLayer || EditLayer->IsLocked() || !EditLayer->SupportsEditingTools())
			{
				continue;
			}

			int64 ExteriorBiomeSum = 0;
			FLandscapeEditDataInterface ScanData(LandscapeInfo, false);
			ScanData.SetShouldDirtyPackage(false);
			ScanData.SetEditLayer(EditLayer->GetGuid());
			for (ULandscapeLayerInfoObject* BiomeLayer : BiomeLayers)
			{
				TArray<uint8> Data;
				ReadLayerRect(ScanData, BiomeLayer, LandscapeRect, Data);
				for (int32 Y = MinY; Y <= MaxY; ++Y)
				{
					for (int32 X = MinX; X <= MaxX; ++X)
					{
						if (!ProtectedRect.Contains(X, Y))
						{
							ExteriorBiomeSum += Data[(X - MinX) + (Y - MinY) * LandscapeRect.Width()];
						}
					}
				}
			}

			UE_LOG(LogTemp, Display, TEXT("[TinoCastleBlend] EditLayer=%s exterior biome sum=%lld"),
				*EditLayer->GetName().ToString(), ExteriorBiomeSum);
			if (ExteriorBiomeSum > LargestExteriorBiomeSum)
			{
				LargestExteriorBiomeSum = ExteriorBiomeSum;
				SourceEditLayerGuid = EditLayer->GetGuid();
				SourceEditLayerName = EditLayer->GetName();
			}
		}

		if (!SourceEditLayerGuid.IsValid() || LargestExteriorBiomeSum == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoCastleBlend] No editable Landscape layer contains the exterior biome paint."));
			return;
		}

		const int32 LayerCount = LandscapeInfo->Layers.Num();
		const int32 VertexCount = LandscapeRect.Width() * LandscapeRect.Height();
		TArray<TArray<uint8>> OriginalLayerData;
		OriginalLayerData.SetNum(LayerCount);
		TArray<int32> WeightBlendLayerIndices;
		TSet<ULandscapeLayerInfoObject*> DirtyLayers;
		{
			FLandscapeEditDataInterface ReadData(LandscapeInfo, false);
			ReadData.SetShouldDirtyPackage(false);
			ReadData.SetEditLayer(SourceEditLayerGuid);
			for (int32 LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
			{
				ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[LayerIndex].LayerInfoObj;
				if (LayerInfo)
				{
					ReadLayerRect(ReadData, LayerInfo, LandscapeRect, OriginalLayerData[LayerIndex]);
					if (LayerInfo != ALandscape::VisibilityLayer &&
						LayerInfo->GetBlendMethod() == ELandscapeTargetLayerBlendMethod::FinalWeightBlending)
					{
						WeightBlendLayerIndices.Add(LayerIndex);
						DirtyLayers.Add(LayerInfo);
					}
				}
				else
				{
					OriginalLayerData[LayerIndex].SetNumZeroed(VertexCount);
				}
			}
		}

		if (WeightBlendLayerIndices.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("[TinoCastleBlend] No Weight-Blended target layers are available."));
			return;
		}

		TArray<uint8> PackedLayerData;
		PackedLayerData.SetNumUninitialized(VertexCount * LayerCount);
		for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
		{
			for (int32 LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
			{
				PackedLayerData[VertexIndex * LayerCount + LayerIndex] = OriginalLayerData[LayerIndex][VertexIndex];
			}
		}

		const FVector Scale = LandscapeToWorld.GetScale3D().GetAbs();
		const double XYScale = FMath::Max(1.0, FMath::Min(Scale.X, Scale.Y));
		const double BaseBlendWidth = BlendWidthWorld / XYScale;
		int32 ChangedVertexCount = 0;
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				if (ProtectedRect.Contains(X, Y))
				{
					continue;
				}

				const double DX = X < ProtectedRect.MinX ? ProtectedRect.MinX - X :
					X > ProtectedRect.MaxX ? X - ProtectedRect.MaxX : 0.0;
				const double DY = Y < ProtectedRect.MinY ? ProtectedRect.MinY - Y :
					Y > ProtectedRect.MaxY ? Y - ProtectedRect.MaxY : 0.0;
				const double Distance = FMath::Sqrt(DX * DX + DY * DY);
				const double BroadWarp = 0.10 * FMath::Sin(static_cast<double>(X + Y) / 43.0 + 0.7)
					+ 0.06 * FMath::Sin(static_cast<double>(X - Y) / 79.0 + 2.1);
				const double EffectiveBlendWidth = BaseBlendWidth * (1.0 + BroadWarp);
				if (Distance > EffectiveBlendWidth)
				{
					continue;
				}

				const int32 InteriorX = FMath::Clamp(X, ProtectedRect.MinX, ProtectedRect.MaxX);
				const int32 InteriorY = FMath::Clamp(Y, ProtectedRect.MinY, ProtectedRect.MaxY);
				const int32 VertexIndex = (X - MinX) + (Y - MinY) * LandscapeRect.Width();
				const int32 InteriorVertexIndex = (InteriorX - MinX) + (InteriorY - MinY) * LandscapeRect.Width();
				const double U = FMath::Clamp(Distance / EffectiveBlendWidth, 0.0, 1.0);
				const double ExteriorAlpha = U * U * (3.0 - 2.0 * U);

				TArray<double, TInlineAllocator<16>> BlendedWeights;
				BlendedWeights.SetNumUninitialized(WeightBlendLayerIndices.Num());
				double WeightSum = 0.0;
				for (int32 BlendIndex = 0; BlendIndex < WeightBlendLayerIndices.Num(); ++BlendIndex)
				{
					const int32 LayerIndex = WeightBlendLayerIndices[BlendIndex];
					const double InteriorWeight = OriginalLayerData[LayerIndex][InteriorVertexIndex];
					const double ExteriorWeight = OriginalLayerData[LayerIndex][VertexIndex];
					BlendedWeights[BlendIndex] = FMath::Lerp(InteriorWeight, ExteriorWeight, ExteriorAlpha);
					WeightSum += BlendedWeights[BlendIndex];
				}

				if (WeightSum <= UE_DOUBLE_SMALL_NUMBER)
				{
					continue;
				}

				int32 QuantizedSum = 0;
				int32 DominantBlendIndex = 0;
				for (int32 BlendIndex = 0; BlendIndex < WeightBlendLayerIndices.Num(); ++BlendIndex)
				{
					const int32 LayerIndex = WeightBlendLayerIndices[BlendIndex];
					const uint8 Quantized = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(BlendedWeights[BlendIndex] * 255.0 / WeightSum), 0, 255));
					PackedLayerData[VertexIndex * LayerCount + LayerIndex] = Quantized;
					QuantizedSum += Quantized;
					if (BlendedWeights[BlendIndex] > BlendedWeights[DominantBlendIndex])
					{
						DominantBlendIndex = BlendIndex;
					}
				}

				const int32 DominantLayerIndex = WeightBlendLayerIndices[DominantBlendIndex];
				uint8& DominantWeight = PackedLayerData[VertexIndex * LayerCount + DominantLayerIndex];
				DominantWeight = static_cast<uint8>(FMath::Clamp(static_cast<int32>(DominantWeight) + 255 - QuantizedSum, 0, 255));
				++ChangedVertexCount;
			}
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT("TinoKingdom", "BlendCastlePaintBoundary", "Blend Castle Landscape Paint Boundary"));
			Landscape->Modify();
			FLandscapeEditDataInterface EditData(LandscapeInfo);
			EditData.SetShouldDirtyPackage(true);
			EditData.SetEditLayer(SourceEditLayerGuid);
			EditData.SetAlphaData(
				DirtyLayers,
				MinX, MinY, MaxX, MaxY,
				PackedLayerData.GetData(),
				LandscapeRect.Width() * LayerCount,
				ELandscapeLayerPaintingRestriction::None);
		}

		Landscape->RequestLayersContentUpdateForceAll(ELandscapeLayerUpdateMode::Update_Weightmap_All, true);
		LandscapeInfo->UpdateAllComponentMaterialInstances(/*bInInvalidateCombinationMaterials=*/true);

		bool bCastleUnchanged = true;
		{
			FLandscapeEditDataInterface VerifyData(LandscapeInfo, false);
			VerifyData.SetShouldDirtyPackage(false);
			VerifyData.SetEditLayer(SourceEditLayerGuid);
			for (int32 LayerIndex = 0; LayerIndex < LayerCount && bCastleUnchanged; ++LayerIndex)
			{
				ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->Layers[LayerIndex].LayerInfoObj;
				if (!LayerInfo)
				{
					continue;
				}
				TArray<uint8> After;
				ReadLayerRect(VerifyData, LayerInfo, ProtectedRect, After);
				for (int32 LocalY = 0; LocalY < ProtectedRect.Height() && bCastleUnchanged; ++LocalY)
				{
					for (int32 LocalX = 0; LocalX < ProtectedRect.Width(); ++LocalX)
					{
						const int32 FullIndex = (ProtectedRect.MinX + LocalX - MinX)
							+ (ProtectedRect.MinY + LocalY - MinY) * LandscapeRect.Width();
						const int32 ProtectedIndex = LocalX + LocalY * ProtectedRect.Width();
						if (After[ProtectedIndex] != OriginalLayerData[LayerIndex][FullIndex])
						{
							bCastleUnchanged = false;
							break;
						}
					}
				}
			}
		}

		if (!bCastleUnchanged)
		{
			GEditor->UndoTransaction();
			UE_LOG(LogTemp, Error, TEXT("[TinoCastleBlend] Verification failed; operation undone because castle weights changed."));
			return;
		}

		Landscape->MarkPackageDirty();
		UE_LOG(LogTemp, Display,
			TEXT("[TinoCastleBlend] SUCCESS. EditLayer=%s ProtectedRect=(%d,%d)-(%d,%d) BlendWidth=%.0fuu ChangedVertices=%d. Castle weights unchanged. Height unchanged."),
			*SourceEditLayerName.ToString(), ProtectedRect.MinX, ProtectedRect.MinY, ProtectedRect.MaxX, ProtectedRect.MaxY,
			BlendWidthWorld, ChangedVertexCount);
	}

	static FAutoConsoleCommand BlendCastlePaintBoundaryCommand(
		TEXT("TinoKingdom.BlendCastlePaintBoundary"),
		TEXT("Smooth the exterior paint into the protected castle paint without changing any castle-interior weights."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&BlendCastlePaintBoundary));
}

#endif // WITH_EDITOR
