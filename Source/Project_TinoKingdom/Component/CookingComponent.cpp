// Fill out your copyright notice in the Description page of Project Settings.



#include "CookingComponent.h"

#include "Engine/Texture2D.h"
#include "Math/UnrealMathUtility.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"

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
	case ECookingTag::Meat:
		return ECookingResultType::Grill;

	case ECookingTag::Mushroom:
	case ECookingTag::Herb:
	case ECookingTag::Monster:
		return ECookingResultType::Soup;

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
	FCookingResultData& OutResult
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

	OutResult = FinishCooking(MinigameScore);
	if (OutResult.ResultType == ECookingResultType::Failed || OutResult.Quality == ECookingQuality::Failed)
	{
		return false;
	}

	UTexture2D* ResultIcon = CreateResultIconTexture(OutResult);

	for (const TPair<FName, int32>& RequiredItemCount : RequiredItemCounts)
	{
		InventoryComponent->RemoveItem(RequiredItemCount.Key, RequiredItemCount.Value);
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
	constexpr int32 TextureSize = 128;
	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(TextureSize * TextureSize);

	const auto ToColor = [](const FLinearColor& Color)
	{
		return Color.ToFColor(true);
	};

	const FColor FishColor = FColor(70, 165, 255, 255);
	const FColor SlimeColor = FColor(55, 235, 125, 255);
	const FColor MushroomColor = FColor(210, 115, 70, 255);
	const FColor MeatColor = FColor(215, 65, 55, 255);
	const FColor HerbColor = FColor(80, 205, 65, 255);
	const FColor WoodColor = FColor(80, 55, 35, 255);
	const FColor MonsterColor = FColor(145, 90, 210, 255);
	const FColor EmptyColor = FColor(0, 0, 0, 0);

	const TArray<TPair<float, FColor>> RatioColors =
	{
		{ ResultData.IconData.FishRatio, FishColor },
		{ ResultData.IconData.SlimeRatio, SlimeColor },
		{ ResultData.IconData.MushroomRatio, MushroomColor },
		{ ResultData.IconData.MeatRatio, MeatColor },
		{ ResultData.IconData.HerbRatio, HerbColor },
		{ ResultData.IconData.WoodRatio, WoodColor },
		{ ResultData.IconData.MonsterRatio, MonsterColor }
	};

	FColor QualityColor = FColor(220, 220, 220, 255);
	switch (ResultData.Quality)
	{
	case ECookingQuality::Good:
		QualityColor = FColor(95, 205, 255, 255);
		break;
	case ECookingQuality::Special:
		QualityColor = FColor(255, 210, 70, 255);
		break;
	default:
		break;
	}

	const FVector2D Center((TextureSize - 1) * 0.5f, (TextureSize - 1) * 0.5f);
	const float OuterRadius = TextureSize * 0.46f;
	const float InnerRadius = TextureSize * 0.17f;
	const float BorderRadius = TextureSize * 0.42f;

	for (int32 Y = 0; Y < TextureSize; ++Y)
	{
		for (int32 X = 0; X < TextureSize; ++X)
		{
			const FVector2D Delta(X - Center.X, Y - Center.Y);
			const float Distance = Delta.Size();
			FColor PixelColor = EmptyColor;

			if (Distance <= OuterRadius)
			{
				if (Distance >= BorderRadius)
				{
					PixelColor = QualityColor;
				}
				else if (Distance <= InnerRadius)
				{
					PixelColor = ToColor(ResultData.IconData.BaseTint);
				}
				else
				{
					float Angle = FMath::Atan2(Delta.Y, Delta.X);
					if (Angle < 0.0f)
					{
						Angle += 2.0f * PI;
					}

					const float RatioPosition = Angle / (2.0f * PI);
					float AccumulatedRatio = 0.0f;
					PixelColor = ToColor(ResultData.IconData.BaseTint);

					for (const TPair<float, FColor>& RatioColor : RatioColors)
					{
						const float Ratio = FMath::Max(RatioColor.Key, 0.0f);
						if (Ratio <= 0.0f)
						{
							continue;
						}

						AccumulatedRatio += Ratio;
						if (RatioPosition <= AccumulatedRatio)
						{
							PixelColor = RatioColor.Value;
							break;
						}
					}
				}
			}

			Pixels[Y * TextureSize + X] = PixelColor;
		}
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8);
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
