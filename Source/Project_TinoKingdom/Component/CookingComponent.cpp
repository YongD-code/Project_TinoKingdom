// Fill out your copyright notice in the Description page of Project Settings.



#include "CookingComponent.h"
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

FCookingResultData UCookingComponent::FinishCooking(float MinigameScore)
{
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
	if (InventoryComponent == nullptr || SelectedIngredients.Num() == 0)
	{
		OutResult = MakeCookingResult(SelectedIngredients);
		return false;
	}

	for (const FInventoryItemStack& Ingredient : SelectedIngredients)
	{
		if (!InventoryComponent->HasItem(Ingredient.ItemId, 1))
		{
			return false;
		}
	}

	OutResult = FinishCooking(MinigameScore);

	for (const FInventoryItemStack& Ingredient : SelectedIngredients)
	{
		InventoryComponent->RemoveItem(Ingredient.ItemId, 1);
	}

	InventoryComponent->AddItem(
		OutResult.ResultItemId,
		OutResult.ResultName,
		1,
		nullptr,
		EInventoryItemType::Food,
		ECookingTag::None,
		EFoodEffectType::None,
		1.0f,
		OutResult
	);

	ClearCookingIngredients();
	return true;
}
