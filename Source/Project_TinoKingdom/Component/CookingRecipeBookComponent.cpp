// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingRecipeBookComponent.h"

UCookingRecipeBookComponent::UCookingRecipeBookComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCookingRecipeBookComponent::RegisterCookingResult(const FCookingResultData& CookingResult, UTexture2D* ResultIcon)
{
	const FName RecipeKey = MakeRecipeKeyFromResult(CookingResult);
	if (RecipeKey.IsNone())
	{
		return false;
	}

	for (FDiscoveredCookingRecipe& Recipe : DiscoveredRecipes)
	{
		if (Recipe.RecipeKey != RecipeKey)
		{
			continue;
		}

		Recipe.TimesCooked += 1;
		if (IsBetterRecipeRecord(Recipe, CookingResult))
		{
			ApplyBestRecord(Recipe, CookingResult, ResultIcon);
		}

		OnRecipeUpdated.Broadcast(Recipe);
		return false;
	}

	FDiscoveredCookingRecipe NewRecipe;
	NewRecipe.RecipeKey = RecipeKey;
	NewRecipe.TimesCooked = 1;
	ApplyBestRecord(NewRecipe, CookingResult, ResultIcon);

	DiscoveredRecipes.Add(NewRecipe);
	OnRecipeDiscovered.Broadcast(DiscoveredRecipes.Last());
	return true;
}

bool UCookingRecipeBookComponent::HasDiscoveredRecipe(FName RecipeKey) const
{
	return DiscoveredRecipes.ContainsByPredicate([RecipeKey](const FDiscoveredCookingRecipe& Recipe)
	{
		return Recipe.RecipeKey == RecipeKey;
	});
}

FName UCookingRecipeBookComponent::MakeRecipeKeyFromResult(const FCookingResultData& CookingResult) const
{
	const FString KeyString = FString::Printf(
		TEXT("Recipe_%d_%d_%d"),
		static_cast<int32>(CookingResult.IconData.MainTag),
		static_cast<int32>(CookingResult.IconData.SubTag),
		static_cast<int32>(CookingResult.ResultType)
	);

	return FName(*KeyString);
}

void UCookingRecipeBookComponent::RestoreRecipesForTravel(const TArray<FDiscoveredCookingRecipe>& SavedRecipes)
{
	DiscoveredRecipes = SavedRecipes;
	DiscoveredRecipes.RemoveAll([](const FDiscoveredCookingRecipe& Recipe)
	{
		return Recipe.RecipeKey.IsNone() || Recipe.TimesCooked <= 0;
	});
}

bool UCookingRecipeBookComponent::IsBetterRecipeRecord(
	const FDiscoveredCookingRecipe& CurrentRecipe,
	const FCookingResultData& CookingResult
)
{
	if (static_cast<int32>(CookingResult.Quality) != static_cast<int32>(CurrentRecipe.BestQuality))
	{
		return static_cast<int32>(CookingResult.Quality) > static_cast<int32>(CurrentRecipe.BestQuality);
	}

	const float CurrentTotal =
		CurrentRecipe.BestHealAmount +
		CurrentRecipe.BestStaminaAmount +
		CurrentRecipe.BestAttackBuffAmount +
		CurrentRecipe.BestDefenseBuffAmount;
	const float NewTotal =
		CookingResult.HealAmount +
		CookingResult.StaminaAmount +
		CookingResult.AttackBuffAmount +
		CookingResult.DefenseBuffAmount;

	return NewTotal > CurrentTotal;
}

void UCookingRecipeBookComponent::ApplyBestRecord(
	FDiscoveredCookingRecipe& Recipe,
	const FCookingResultData& CookingResult,
	UTexture2D* ResultIcon
)
{
	Recipe.ResultName = CookingResult.ResultName;
	Recipe.ResultType = CookingResult.ResultType;
	Recipe.BestQuality = CookingResult.Quality;
	Recipe.BestHealAmount = CookingResult.HealAmount;
	Recipe.BestStaminaAmount = CookingResult.StaminaAmount;
	Recipe.BestAttackBuffAmount = CookingResult.AttackBuffAmount;
	Recipe.BestDefenseBuffAmount = CookingResult.DefenseBuffAmount;
	Recipe.IconData = CookingResult.IconData;
	Recipe.Icon = ResultIcon;
}
