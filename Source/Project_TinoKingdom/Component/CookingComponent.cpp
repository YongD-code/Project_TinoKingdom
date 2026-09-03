// Fill out your copyright notice in the Description page of Project Settings.



#include "CookingComponent.h"

#include "Engine/Texture2D.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Guid.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"

namespace
{
constexpr int32 CookingIconTextureSize = 256;

struct FCookingTexturePixels
{
	int32 Width = 0;
	int32 Height = 0;
	TArray<FColor> Pixels;

	bool IsValid() const
	{
		return Width > 0 && Height > 0 && Pixels.Num() == Width * Height;
	}
};

struct FCookingPlacementBounds
{
	float MinX = 0.25f;
	float MaxX = 0.75f;
	float MinY = 0.25f;
	float MaxY = 0.75f;
	float MinDistance = 0.13f;
	float ScaleMultiplier = 1.0f;
};

UTexture2D* LoadCookingLayerTexture(const TCHAR* AssetPath)
{
	return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, AssetPath));
}

bool ReadTextureSourcePixels(UTexture2D* Texture, FCookingTexturePixels& OutPixels)
{
	if (Texture == nullptr)
	{
		return false;
	}

#if WITH_EDITORONLY_DATA
	if (Texture->Source.IsValid())
	{
		TArray64<uint8> SourceData;
		Texture->Source.GetMipData(SourceData, 0);

		const int32 Width = Texture->Source.GetSizeX();
		const int32 Height = Texture->Source.GetSizeY();
		if (Width <= 0 || Height <= 0)
		{
			return false;
		}

		const ETextureSourceFormat SourceFormat = Texture->Source.GetFormat();
		OutPixels.Width = Width;
		OutPixels.Height = Height;
		OutPixels.Pixels.SetNumZeroed(Width * Height);

		if (SourceFormat == TSF_BGRA8 && SourceData.Num() >= Width * Height * 4)
		{
			FMemory::Memcpy(OutPixels.Pixels.GetData(), SourceData.GetData(), Width * Height * sizeof(FColor));
			return true;
		}

	}
#endif

	return false;
}

void AlphaBlendPixel(FColor& Target, const FColor& Source, float Opacity)
{
	const float SourceAlpha = (Source.A / 255.0f) * FMath::Clamp(Opacity, 0.0f, 1.0f);
	if (SourceAlpha <= 0.0f)
	{
		return;
	}

	const float TargetAlpha = Target.A / 255.0f;
	const float OutAlpha = SourceAlpha + TargetAlpha * (1.0f - SourceAlpha);
	if (OutAlpha <= 0.0f)
	{
		Target = FColor(0, 0, 0, 0);
		return;
	}

	const auto BlendChannel = [SourceAlpha, TargetAlpha, OutAlpha](uint8 SourceValue, uint8 TargetValue)
	{
		const float SourceLinear = SourceValue / 255.0f;
		const float TargetLinear = TargetValue / 255.0f;
		return static_cast<uint8>(FMath::Clamp(
			((SourceLinear * SourceAlpha) + (TargetLinear * TargetAlpha * (1.0f - SourceAlpha))) / OutAlpha * 255.0f,
			0.0f,
			255.0f
		));
	};

	Target.R = BlendChannel(Source.R, Target.R);
	Target.G = BlendChannel(Source.G, Target.G);
	Target.B = BlendChannel(Source.B, Target.B);
	Target.A = static_cast<uint8>(FMath::Clamp(OutAlpha * 255.0f, 0.0f, 255.0f));
}

void BlendTextureLayer(TArray<FColor>& TargetPixels, int32 TargetSize, const FCookingTexturePixels& SourcePixels, float Opacity)
{
	if (!SourcePixels.IsValid() || TargetPixels.Num() != TargetSize * TargetSize || Opacity <= 0.0f)
	{
		return;
	}

	for (int32 Y = 0; Y < TargetSize; ++Y)
	{
		const int32 SourceY = FMath::Clamp((Y * SourcePixels.Height) / TargetSize, 0, SourcePixels.Height - 1);
		for (int32 X = 0; X < TargetSize; ++X)
		{
			const int32 SourceX = FMath::Clamp((X * SourcePixels.Width) / TargetSize, 0, SourcePixels.Width - 1);
			AlphaBlendPixel(TargetPixels[Y * TargetSize + X], SourcePixels.Pixels[SourceY * SourcePixels.Width + SourceX], Opacity);
		}
	}
}

void BlendCookingLayer(TArray<FColor>& TargetPixels, const TCHAR* AssetPath, float Opacity)
{
	FCookingTexturePixels LayerPixels;
	if (ReadTextureSourcePixels(LoadCookingLayerTexture(AssetPath), LayerPixels))
	{
		BlendTextureLayer(TargetPixels, CookingIconTextureSize, LayerPixels, Opacity);
	}
}

void BlendTextureLayerAt(
	TArray<FColor>& TargetPixels,
	const FCookingTexturePixels& SourcePixels,
	float CenterX,
	float CenterY,
	float Scale,
	float Opacity
)
{
	if (!SourcePixels.IsValid() || TargetPixels.Num() != CookingIconTextureSize * CookingIconTextureSize || Scale <= 0.0f || Opacity <= 0.0f)
	{
		return;
	}

	const int32 DrawWidth = FMath::Max(1, FMath::RoundToInt(CookingIconTextureSize * Scale));
	const int32 DrawHeight = DrawWidth;
	const int32 MinX = FMath::RoundToInt(CenterX * CookingIconTextureSize) - DrawWidth / 2;
	const int32 MinY = FMath::RoundToInt(CenterY * CookingIconTextureSize) - DrawHeight / 2;
	const int32 MaxX = MinX + DrawWidth;
	const int32 MaxY = MinY + DrawHeight;

	for (int32 Y = FMath::Max(0, MinY); Y < FMath::Min(CookingIconTextureSize, MaxY); ++Y)
	{
		const int32 SourceY = FMath::Clamp(((Y - MinY) * SourcePixels.Height) / DrawHeight, 0, SourcePixels.Height - 1);
		for (int32 X = FMath::Max(0, MinX); X < FMath::Min(CookingIconTextureSize, MaxX); ++X)
		{
			const int32 SourceX = FMath::Clamp(((X - MinX) * SourcePixels.Width) / DrawWidth, 0, SourcePixels.Width - 1);
			AlphaBlendPixel(TargetPixels[Y * CookingIconTextureSize + X], SourcePixels.Pixels[SourceY * SourcePixels.Width + SourceX], Opacity);
		}
	}
}

void BlendCookingLayerAt(
	TArray<FColor>& TargetPixels,
	const TCHAR* AssetPath,
	float CenterX,
	float CenterY,
	float Scale,
	float Opacity
)
{
	FCookingTexturePixels LayerPixels;
	if (ReadTextureSourcePixels(LoadCookingLayerTexture(AssetPath), LayerPixels))
	{
		BlendTextureLayerAt(TargetPixels, LayerPixels, CenterX, CenterY, Scale, Opacity);
	}
}

void BlendTintLayer(TArray<FColor>& TargetPixels, const FColor& TintColor, float Opacity)
{
	const FVector2D Center((CookingIconTextureSize - 1) * 0.5f, (CookingIconTextureSize - 1) * 0.5f);
	const float Radius = CookingIconTextureSize * 0.34f;

	for (int32 Y = 0; Y < CookingIconTextureSize; ++Y)
	{
		for (int32 X = 0; X < CookingIconTextureSize; ++X)
		{
			if (FVector2D(X - Center.X, Y - Center.Y).Size() <= Radius)
			{
				AlphaBlendPixel(TargetPixels[Y * CookingIconTextureSize + X], TintColor, Opacity);
			}
		}
	}
}

float GetVisibleRatioOpacity(float Ratio)
{
	if (Ratio <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(0.5f + Ratio * 0.5f, 0.0f, 1.0f);
}

const TCHAR* GetCookingBaseAssetPath(ECookingResultType ResultType)
{
	switch (ResultType)
	{
	case ECookingResultType::Jelly:
		return TEXT("/Game/Cooking/Assets/Base_Jelly.Base_Jelly");
	case ECookingResultType::Soup:
		return TEXT("/Game/Cooking/Assets/Base_Soup.Base_Soup");
	case ECookingResultType::Grill:
		return TEXT("/Game/Cooking/Assets/Base_Grill.Base_Grill");
	case ECookingResultType::Failed:
	default:
		return TEXT("/Game/Cooking/Assets/Base_Failed.Base_Failed");
	}
}

const TCHAR* GetCookingFrameAssetPath(ECookingQuality Quality)
{
	switch (Quality)
	{
	case ECookingQuality::Good:
		return TEXT("/Game/Cooking/Assets/Frame_Good.Frame_Good");
	case ECookingQuality::Special:
		return TEXT("/Game/Cooking/Assets/Frame_Rare.Frame_Rare");
	case ECookingQuality::Failed:
		return TEXT("/Game/Cooking/Assets/Frame_Failed.Frame_Failed");
	case ECookingQuality::Normal:
	default:
		return TEXT("/Game/Cooking/Assets/Frame_Normal.Frame_Normal");
	}
}

const TCHAR* GetCookingChunkAssetPath(ECookingTag Tag)
{
	switch (Tag)
	{
	case ECookingTag::Fish:
		return TEXT("/Game/Cooking/Assets/Chunk_Fish.Chunk_Fish");
	case ECookingTag::Slime:
		return TEXT("/Game/Cooking/Assets/Chunk_SlimeBubble.Chunk_SlimeBubble");
	case ECookingTag::Mushroom:
		return TEXT("/Game/Cooking/Assets/Chunk_MushroomSlice.Chunk_MushroomSlice");
	case ECookingTag::Meat:
		return TEXT("/Game/Cooking/Assets/Chunk_MeatCube.Chunk_MeatCube");
	default:
		return nullptr;
	}
}

const TCHAR* GetCookingToppingAssetPath(ECookingTag Tag)
{
	switch (Tag)
	{
	case ECookingTag::Fish:
		return nullptr;
	case ECookingTag::Slime:
		return TEXT("/Game/Cooking/Assets/Topping_SlimeDrop.Topping_SlimeDrop");
	case ECookingTag::Mushroom:
		return TEXT("/Game/Cooking/Assets/Topping_Mushroom.Topping_Mushroom");
	case ECookingTag::Meat:
		return TEXT("/Game/Cooking/Assets/Topping_Meat.Topping_Meat");
	case ECookingTag::Herb:
		return TEXT("/Game/Cooking/Assets/Topping_Herb.Topping_Herb");
	default:
		return nullptr;
	}
}

float GetCookingPieceScaleMultiplier(ECookingTag Tag, bool bMainIngredient)
{
	switch (Tag)
	{
	case ECookingTag::Fish:
		return bMainIngredient ? 0.66f : 0.58f;
	case ECookingTag::Meat:
		return bMainIngredient ? 0.88f : 0.76f;
	case ECookingTag::Mushroom:
		return bMainIngredient ? 0.82f : 0.72f;
	case ECookingTag::Slime:
		return bMainIngredient ? 1.0f : 0.86f;
	default:
		return 1.0f;
	}
}

FCookingPlacementBounds GetCookingPlacementBounds(
	ECookingResultType ResultType,
	bool bMainIngredient
)
{
	switch (ResultType)
	{
	case ECookingResultType::Soup:
		return bMainIngredient
			? FCookingPlacementBounds{ 0.32f, 0.68f, 0.43f, 0.67f, 0.12f, 0.68f }
			: FCookingPlacementBounds{ 0.35f, 0.65f, 0.39f, 0.62f, 0.14f, 0.56f };

	case ECookingResultType::Jelly:
		return bMainIngredient
			? FCookingPlacementBounds{ 0.33f, 0.67f, 0.29f, 0.51f, 0.13f, 0.62f }
			: FCookingPlacementBounds{ 0.37f, 0.63f, 0.24f, 0.42f, 0.15f, 0.50f };

	case ECookingResultType::Grill:
		return bMainIngredient
			? FCookingPlacementBounds{ 0.30f, 0.70f, 0.42f, 0.68f, 0.13f, 0.74f }
			: FCookingPlacementBounds{ 0.35f, 0.65f, 0.34f, 0.56f, 0.15f, 0.56f };

	case ECookingResultType::Failed:
	default:
		return bMainIngredient
			? FCookingPlacementBounds{ 0.34f, 0.66f, 0.40f, 0.65f, 0.13f, 0.64f }
			: FCookingPlacementBounds{ 0.37f, 0.63f, 0.35f, 0.57f, 0.15f, 0.50f };
	}
}

float GetCookingTagRatio(const FCookingIconData& IconData, ECookingTag Tag)
{
	switch (Tag)
	{
	case ECookingTag::Fish:
		return IconData.FishRatio;
	case ECookingTag::Slime:
		return IconData.SlimeRatio;
	case ECookingTag::Mushroom:
		return IconData.MushroomRatio;
	case ECookingTag::Meat:
		return IconData.MeatRatio;
	case ECookingTag::Herb:
		return IconData.HerbRatio;
	case ECookingTag::Wood:
		return IconData.WoodRatio;
	case ECookingTag::Monster:
		return IconData.MonsterRatio;
	default:
		return 0.0f;
	}
}

FColor GetFallbackTintByTag(ECookingTag Tag)
{
	switch (Tag)
	{
	case ECookingTag::Wood:
		return FColor(80, 55, 35, 255);
	case ECookingTag::Monster:
		return FColor(145, 90, 210, 255);
	default:
		return FColor(255, 255, 255, 0);
	}
}

FColor GetAccentColorByTag(ECookingTag Tag)
{
	switch (Tag)
	{
	case ECookingTag::Fish:
		return FColor(115, 205, 255, 255);
	case ECookingTag::Slime:
		return FColor(90, 245, 215, 255);
	case ECookingTag::Mushroom:
		return FColor(220, 115, 85, 255);
	case ECookingTag::Meat:
		return FColor(210, 70, 45, 255);
	case ECookingTag::Herb:
		return FColor(95, 225, 90, 255);
	case ECookingTag::Wood:
		return FColor(55, 35, 25, 255);
	case ECookingTag::Monster:
		return FColor(160, 95, 220, 255);
	default:
		return FColor(255, 255, 255, 0);
	}
}

void BlendMainIngredientLayer(TArray<FColor>& Pixels, ECookingTag MainTag)
{
	if (const TCHAR* ChunkPath = GetCookingChunkAssetPath(MainTag))
	{
		BlendCookingLayer(Pixels, ChunkPath, 1.0f);
		return;
	}

	if (const TCHAR* ToppingPath = GetCookingToppingAssetPath(MainTag))
	{
		BlendCookingLayer(Pixels, ToppingPath, 0.9f);
		return;
	}

	BlendTintLayer(Pixels, GetFallbackTintByTag(MainTag), 0.55f);
}

void BlendIngredientPieces(
	TArray<FColor>& Pixels,
	ECookingResultType ResultType,
	ECookingTag Tag,
	float Ratio,
	bool bMainIngredient
)
{
	if (Ratio <= 0.0f)
	{
		return;
	}

	const int32 PieceCount = bMainIngredient
		? FMath::Clamp(FMath::RoundToInt(1.5f + Ratio * 4.5f + FMath::FRandRange(-0.5f, 0.75f)), 2, 5)
		: FMath::Clamp(FMath::RoundToInt(0.5f + Ratio * 3.5f + FMath::FRandRange(-0.35f, 0.55f)), 1, 3);
	const float Opacity = bMainIngredient
		? FMath::Clamp(0.66f + Ratio * 0.28f, 0.0f, 0.94f)
		: FMath::Clamp(0.74f + Ratio * 0.22f, 0.0f, 0.96f);
	const float BaseScale = bMainIngredient
		? FMath::Lerp(0.17f, 0.27f, Ratio)
		: FMath::Lerp(0.13f, 0.20f, Ratio);
	const float PieceScaleMultiplier = GetCookingPieceScaleMultiplier(Tag, bMainIngredient);
	const FCookingPlacementBounds Bounds = GetCookingPlacementBounds(ResultType, bMainIngredient);

	const TCHAR* PiecePath = GetCookingChunkAssetPath(Tag);
	if (PiecePath == nullptr)
	{
		PiecePath = GetCookingToppingAssetPath(Tag);
	}

	if (PiecePath == nullptr)
	{
		BlendTintLayer(Pixels, GetFallbackTintByTag(Tag), Opacity * 0.45f);
		return;
	}

	TArray<FVector2D> UsedPositions;
	for (int32 Index = 0; Index < PieceCount; ++Index)
	{
		FVector2D Position(0.5f, 0.5f);

		for (int32 Attempt = 0; Attempt < 12; ++Attempt)
		{
			const float CenterX = FMath::FRandRange(Bounds.MinX, Bounds.MaxX);
			const float CenterY = FMath::FRandRange(Bounds.MinY, Bounds.MaxY);
			const FVector2D Candidate(CenterX, CenterY);

			bool bTooClose = false;
			for (const FVector2D& UsedPosition : UsedPositions)
			{
				if (FVector2D::Distance(Candidate, UsedPosition) < Bounds.MinDistance)
				{
					bTooClose = true;
					break;
				}
			}

			Position = Candidate;
			if (!bTooClose)
			{
				break;
			}
		}

		UsedPositions.Add(Position);

		const float ScaleJitter = FMath::FRandRange(0.86f, 1.12f);
		const float EdgeScale = bMainIngredient ? FMath::Lerp(1.04f, 0.88f, Index / static_cast<float>(FMath::Max(PieceCount - 1, 1))) : 1.0f;
		BlendCookingLayerAt(
			Pixels,
			PiecePath,
			Position.X,
			Position.Y,
			BaseScale * Bounds.ScaleMultiplier * PieceScaleMultiplier * ScaleJitter * EdgeScale,
			Opacity * FMath::FRandRange(0.88f, 1.0f)
		);
	}

	if (!bMainIngredient)
	{
		if (const TCHAR* ToppingPath = GetCookingToppingAssetPath(Tag))
		{
			const float AccentX = FMath::FRandRange(Bounds.MinX, Bounds.MaxX);
			const float AccentY = FMath::FRandRange(Bounds.MinY, Bounds.MaxY);
			BlendCookingLayerAt(Pixels, ToppingPath, AccentX, AccentY, BaseScale * Bounds.ScaleMultiplier * FMath::FRandRange(0.42f, 0.66f), Opacity * 0.82f);
		}
	}
}
}

UCookingComponent::UCookingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCookingComponent::BeginPlay()
{
	Super::BeginPlay();
}

FCookingResultData UCookingComponent::MakeCookingResult(const TArray<FInventoryItemStack>& Ingredients) const
{
	FCookingResultData Result;

	if (Ingredients.Num() == 0)
	{
		Result.ResultType = ECookingResultType::Failed;
		Result.Quality = ECookingQuality::Failed;
		Result.ResultName = FText::FromString(TEXT("빈 요리"));
		Result.ResultItemId = FName(TEXT("Food_Empty"));
		return Result;
	}

	TMap<ECookingTag, float> TagAmounts;
	float TotalAmount = 0.0f;

	for (const FInventoryItemStack& Ingredient : Ingredients)
	{
		if (Ingredient.ItemType != EInventoryItemType::Material)
		{
			continue;
		}

		if (Ingredient.CookingTag == ECookingTag::None)
		{
			continue;
		}

		const float Amount = FMath::Max(Ingredient.CookingPower, 1.0f);
		TagAmounts.FindOrAdd(Ingredient.CookingTag) += Amount;
		TotalAmount += Amount;
	}

	if (TotalAmount <= 0.0f)
	{
		Result.ResultType = ECookingResultType::Failed;
		Result.Quality = ECookingQuality::Failed;
		Result.ResultName = FText::FromString(TEXT("실패한 요리"));
		Result.ResultItemId = FName(TEXT("Food_Failed"));
		return Result;
	}

	TMap<ECookingTag, float> TagRatios;

	for (const TPair<ECookingTag, float>& Pair : TagAmounts)
	{
		TagRatios.Add(Pair.Key, Pair.Value / TotalAmount);
	}

	const ECookingTag MainTag = FindMainTag(TagRatios);
	const ECookingTag SubTag = FindSubTag(TagRatios, MainTag);

	Result.ResultType = GetResultTypeByMainTag(MainTag);
	Result.Quality = ECookingQuality::Normal;

	Result.HealAmount = TagRatios.FindRef(ECookingTag::Fish) * 40.0f;
	Result.StaminaAmount = TagRatios.FindRef(ECookingTag::Slime) * 40.0f;
	Result.DefenseBuffAmount = TagRatios.FindRef(ECookingTag::Mushroom) * 10.0f;
	Result.AttackBuffAmount = TagRatios.FindRef(ECookingTag::Meat) * 10.0f;

	Result.ResultName = MakeResultName(MainTag, SubTag, Result.ResultType, Result.Quality);
	Result.ResultItemId = MakeResultItemId(MainTag, SubTag, Result.ResultType, Result.Quality);

	Result.IconData.BaseType = Result.ResultType;
	Result.IconData.MainTag = MainTag;
	Result.IconData.SubTag = SubTag;
	Result.IconData.BaseTint = GetBaseTintByMainTag(MainTag);
	Result.IconData.FishRatio = TagRatios.FindRef(ECookingTag::Fish);
	Result.IconData.SlimeRatio = TagRatios.FindRef(ECookingTag::Slime);
	Result.IconData.MushroomRatio = TagRatios.FindRef(ECookingTag::Mushroom);
	Result.IconData.MeatRatio = TagRatios.FindRef(ECookingTag::Meat);
	Result.IconData.HerbRatio = TagRatios.FindRef(ECookingTag::Herb);
	Result.IconData.WoodRatio = TagRatios.FindRef(ECookingTag::Wood);
	Result.IconData.MonsterRatio = TagRatios.FindRef(ECookingTag::Monster);

	return Result;
}

ECookingTag UCookingComponent::FindMainTag(const TMap<ECookingTag, float>& TagRatios) const
{
	ECookingTag BestTag = ECookingTag::None;
	float BestRatio = 0.0f;

	for (const TPair<ECookingTag, float>& Pair : TagRatios)
	{
		if (Pair.Value > BestRatio)
		{
			BestRatio = Pair.Value;
			BestTag = Pair.Key;
		}
	}

	return BestTag;
}

ECookingTag UCookingComponent::FindSubTag(
	const TMap<ECookingTag, float>& TagRatios,
	ECookingTag MainTag
) const
{
	ECookingTag BestTag = ECookingTag::None;
	float BestRatio = 0.0f;

	for (const TPair<ECookingTag, float>& Pair : TagRatios)
	{
		if (Pair.Key == MainTag)
		{
			continue;
		}

		if (Pair.Value > BestRatio)
		{
			BestRatio = Pair.Value;
			BestTag = Pair.Key;
		}
	}

	return BestTag;
}

ECookingResultType UCookingComponent::GetResultTypeByMainTag(ECookingTag MainTag) const
{
	switch (MainTag)
	{
	case ECookingTag::Slime:
		return ECookingResultType::Jelly;

	case ECookingTag::Fish:
	case ECookingTag::Mushroom:
	case ECookingTag::Herb:
	case ECookingTag::Monster:
		return ECookingResultType::Soup;

	case ECookingTag::Meat:
		return ECookingResultType::Grill;

	case ECookingTag::Wood:
		return ECookingResultType::Failed;

	default:
		return ECookingResultType::Failed;
	}
}

FLinearColor UCookingComponent::GetBaseTintByMainTag(ECookingTag MainTag) const
{
	switch (MainTag)
	{
	case ECookingTag::Slime:
		return FLinearColor(0.1f, 0.9f, 0.45f, 1.0f);

	case ECookingTag::Fish:
		return FLinearColor(0.2f, 0.65f, 1.0f, 1.0f);

	case ECookingTag::Mushroom:
		return FLinearColor(0.85f, 0.45f, 0.25f, 1.0f);

	case ECookingTag::Meat:
		return FLinearColor(0.8f, 0.25f, 0.15f, 1.0f);

	case ECookingTag::Herb:
		return FLinearColor(0.25f, 0.85f, 0.2f, 1.0f);

	case ECookingTag::Monster:
		return FLinearColor(0.45f, 0.25f, 0.75f, 1.0f);

	case ECookingTag::Wood:
		return FLinearColor(0.12f, 0.08f, 0.04f, 1.0f);

	default:
		return FLinearColor::White;
	}
}

ECookingQuality UCookingComponent::GetQualityByMinigameScore(float MinigameScore) const
{
	const float ClampedScore = FMath::Clamp(MinigameScore, 0.0f, 100.0f);

	if (ClampedScore < 40.0f)
	{
		return ECookingQuality::Failed;
	}

	if (ClampedScore < 70.0f)
	{
		return ECookingQuality::Normal;
	}

	if (ClampedScore < 90.0f)
	{
		return ECookingQuality::Good;
	}

	return ECookingQuality::Special;
}

FText UCookingComponent::MakeResultName(
	ECookingTag MainTag,
	ECookingTag SubTag,
	ECookingResultType ResultType,
	ECookingQuality Quality
) const
{
	if (Quality == ECookingQuality::Failed || ResultType == ECookingResultType::Failed)
	{
		return FText::FromString(TEXT("실패한 요리"));
	}

	const FText MainName = GetTagDisplayName(MainTag);
	const FText SubName = GetTagDisplayName(SubTag);
	const FText TypeName = GetResultTypeDisplayName(ResultType);

	if (SubTag == ECookingTag::None)
	{
		return FText::Format(
			FText::FromString(TEXT("{0} {1}")),
			MainName,
			TypeName
		);
	}

	return FText::Format(
		FText::FromString(TEXT("{0} {1} {2}")),
		MainName,
		SubName,
		TypeName
	);
}

FName UCookingComponent::MakeResultItemId(
	ECookingTag MainTag,
	ECookingTag SubTag,
	ECookingResultType ResultType,
	ECookingQuality Quality
) const
{
	const FString ItemIdString = FString::Printf(
		TEXT("Food_%d_%d_%d_%d"),
		static_cast<int32>(MainTag),
		static_cast<int32>(SubTag),
		static_cast<int32>(ResultType),
		static_cast<int32>(Quality)
	);

	return FName(*ItemIdString);
}

float UCookingComponent::GetQualityMultiplier(ECookingQuality Quality) const
{
	switch (Quality)
	{
	case ECookingQuality::Failed:
		return 0.5f;

	case ECookingQuality::Normal:
		return 1.0f;

	case ECookingQuality::Good:
		return 1.3f;

	case ECookingQuality::Special:
		return 1.6f;

	default:
		return 1.0f;
	}
}

FText UCookingComponent::GetTagDisplayName(ECookingTag Tag) const
{
	switch (Tag)
	{
	case ECookingTag::Fish:
		return FText::FromString(TEXT("생선"));

	case ECookingTag::Slime:
		return FText::FromString(TEXT("슬라임"));

	case ECookingTag::Mushroom:
		return FText::FromString(TEXT("버섯"));

	case ECookingTag::Meat:
		return FText::FromString(TEXT("고기"));

	case ECookingTag::Herb:
		return FText::FromString(TEXT("약초"));

	case ECookingTag::Wood:
		return FText::FromString(TEXT("탄"));

	case ECookingTag::Monster:
		return FText::FromString(TEXT("몬스터"));

	default:
		return FText::GetEmpty();
	}
}

FText UCookingComponent::GetResultTypeDisplayName(ECookingResultType ResultType) const
{
	switch (ResultType)
	{
	case ECookingResultType::Jelly:
		return FText::FromString(TEXT("젤리"));

	case ECookingResultType::Soup:
		return FText::FromString(TEXT("수프"));

	case ECookingResultType::Grill:
		return FText::FromString(TEXT("구이"));

	case ECookingResultType::Failed:
		return FText::FromString(TEXT("요리"));

	default:
		return FText::FromString(TEXT("요리"));
	}
}

bool UCookingComponent::AddCookingIngredient(const FInventoryItemStack& Ingredient)
{
	if (SelectedIngredients.Num() >= MaxIngredientCount)
	{
		return false;
	}

	if (Ingredient.Count <= 0)
	{
		return false;
	}

	if (Ingredient.ItemType != EInventoryItemType::Material)
	{
		return false;
	}

	FInventoryItemStack IngredientToAdd = Ingredient;
	IngredientToAdd.Count = 1;

	SelectedIngredients.Add(IngredientToAdd);
	return true;
}

void UCookingComponent::RemoveCookingIngredientAt(int32 Index)
{
	if (SelectedIngredients.IsValidIndex(Index))
	{
		SelectedIngredients.RemoveAt(Index);
	}
}

void UCookingComponent::ClearCookingIngredients()
{
	SelectedIngredients.Empty();
}

const TArray<FInventoryItemStack>& UCookingComponent::GetSelectedIngredients() const
{
	return SelectedIngredients;
}

bool UCookingComponent::CanFinishCooking() const
{
	return SelectedIngredients.Num() >= MaxIngredientCount;
}

FCookingResultData UCookingComponent::FinishCooking(float MinigameScore)
{
	if (!CanFinishCooking())
	{
		FCookingResultData Result;
		Result.ResultType = ECookingResultType::Failed;
		Result.Quality = ECookingQuality::Failed;
		Result.ResultName = FText::FromString(TEXT("재료가 부족합니다"));
		Result.ResultItemId = FName(TEXT("Food_NotEnoughIngredients"));
		return Result;
	}

	FCookingResultData Result = MakeCookingResult(SelectedIngredients);

	if (Result.ResultType == ECookingResultType::Failed)
	{
		return Result;
	}

	Result.Quality = GetQualityByMinigameScore(MinigameScore);

	const float QualityMultiplier = GetQualityMultiplier(Result.Quality);

	Result.HealAmount *= QualityMultiplier;
	Result.StaminaAmount *= QualityMultiplier;
	Result.AttackBuffAmount *= QualityMultiplier;
	Result.DefenseBuffAmount *= QualityMultiplier;

	const ECookingTag MainTag = Result.IconData.MainTag;
	const ECookingTag SubTag = Result.IconData.SubTag;

	Result.ResultName = MakeResultName(
		MainTag,
		SubTag,
		Result.ResultType,
		Result.Quality
	);

	Result.ResultItemId = MakeResultItemId(
		MainTag,
		SubTag,
		Result.ResultType,
		Result.Quality
	);

	return Result;
}

bool UCookingComponent::FinishCookingToInventory(
	UInventoryComponent* InventoryComponent,
	float MinigameScore,
	FCookingResultData& OutResult,
	bool bConsumeIngredients
)
{
	if (InventoryComponent == nullptr || !CanFinishCooking())
	{
		OutResult = FCookingResultData();
		OutResult.ResultType = ECookingResultType::Failed;
		OutResult.Quality = ECookingQuality::Failed;
		OutResult.ResultName = FText::FromString(TEXT("재료가 부족합니다"));
		OutResult.ResultItemId = FName(TEXT("Food_NotEnoughIngredients"));
		return false;
	}

	TMap<FName, int32> RequiredItemCounts;
	for (const FInventoryItemStack& Ingredient : SelectedIngredients)
	{
		RequiredItemCounts.FindOrAdd(Ingredient.ItemId) += 1;
	}

	if (bConsumeIngredients)
	{
		for (const TPair<FName, int32>& RequiredItemCount : RequiredItemCounts)
		{
			if (!InventoryComponent->HasItem(RequiredItemCount.Key, RequiredItemCount.Value))
			{
				OutResult = FCookingResultData();
				OutResult.ResultType = ECookingResultType::Failed;
				OutResult.Quality = ECookingQuality::Failed;
				OutResult.ResultName = FText::FromString(TEXT("재료가 부족합니다"));
				OutResult.ResultItemId = FName(TEXT("Food_NotEnoughIngredients"));
				return false;
			}
		}
	}

	OutResult = FinishCooking(MinigameScore);
	if (OutResult.ResultType == ECookingResultType::Failed || OutResult.Quality == ECookingQuality::Failed)
	{
		return false;
	}

	OutResult.ResultItemId = FName(*FString::Printf(
		TEXT("%s_%s"),
		*OutResult.ResultItemId.ToString(),
		*FGuid::NewGuid().ToString(EGuidFormats::Short)
	));

	UTexture2D* ResultIcon = CreateResultIconTexture(OutResult);

	if (bConsumeIngredients)
	{
		for (const TPair<FName, int32>& RequiredItemCount : RequiredItemCounts)
		{
			InventoryComponent->RemoveItem(RequiredItemCount.Key, RequiredItemCount.Value);
		}
	}

	InventoryComponent->AddItem(
		OutResult.ResultItemId,
		OutResult.ResultName,
		1,
		ResultIcon,
		EInventoryItemType::Food,
		ECookingTag::None,
		EFoodEffectType::None,
		1.0f,
		OutResult
	);

	ClearCookingIngredients();
	return true;
}

UTexture2D* UCookingComponent::CreateResultIconTexture(const FCookingResultData& ResultData) const
{
	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(CookingIconTextureSize * CookingIconTextureSize);

	BlendCookingLayer(Pixels, GetCookingBaseAssetPath(ResultData.ResultType), 1.0f);
	BlendTintLayer(Pixels, GetAccentColorByTag(ResultData.IconData.MainTag), 0.10f);
	BlendIngredientPieces(
		Pixels,
		ResultData.ResultType,
		ResultData.IconData.MainTag,
		GetCookingTagRatio(ResultData.IconData, ResultData.IconData.MainTag),
		true
	);

	TArray<TPair<ECookingTag, float>> SubIngredientRatios =
	{
		{ ECookingTag::Fish, GetCookingTagRatio(ResultData.IconData, ECookingTag::Fish) },
		{ ECookingTag::Slime, GetCookingTagRatio(ResultData.IconData, ECookingTag::Slime) },
		{ ECookingTag::Mushroom, GetCookingTagRatio(ResultData.IconData, ECookingTag::Mushroom) },
		{ ECookingTag::Meat, GetCookingTagRatio(ResultData.IconData, ECookingTag::Meat) },
		{ ECookingTag::Herb, GetCookingTagRatio(ResultData.IconData, ECookingTag::Herb) },
		{ ECookingTag::Wood, GetCookingTagRatio(ResultData.IconData, ECookingTag::Wood) },
		{ ECookingTag::Monster, GetCookingTagRatio(ResultData.IconData, ECookingTag::Monster) }
	};

	SubIngredientRatios.Sort([](const TPair<ECookingTag, float>& Left, const TPair<ECookingTag, float>& Right)
	{
		return Left.Value > Right.Value;
	});

	for (const TPair<ECookingTag, float>& Pair : SubIngredientRatios)
	{
		if (Pair.Key == ResultData.IconData.MainTag || Pair.Value <= 0.0f)
		{
			continue;
		}

		BlendIngredientPieces(Pixels, ResultData.ResultType, Pair.Key, Pair.Value, false);
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(CookingIconTextureSize, CookingIconTextureSize, PF_B8G8R8A8);
	if (Texture == nullptr || Texture->GetPlatformData() == nullptr || Texture->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	Texture->NeverStream = true;
	Texture->CompressionSettings = TC_VectorDisplacementmap;
	Texture->SRGB = true;

	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();
	Texture->UpdateResource();

	return Texture;
}
