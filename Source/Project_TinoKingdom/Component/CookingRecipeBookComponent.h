// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Project_TinoKingdom/Types/CookingTypes.h"
#include "CookingRecipeBookComponent.generated.h"

class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingRecipeDiscovered, const FDiscoveredCookingRecipe&, Recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingRecipeUpdated, const FDiscoveredCookingRecipe&, Recipe);

UCLASS(ClassGroup=(Tino), meta=(BlueprintSpawnableComponent))
class PROJECT_TINOKINGDOM_API UCookingRecipeBookComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCookingRecipeBookComponent();

	UFUNCTION(BlueprintCallable, Category = "Cooking Recipe Book")
	bool RegisterCookingResult(const FCookingResultData& CookingResult, UTexture2D* ResultIcon);

	UFUNCTION(BlueprintPure, Category = "Cooking Recipe Book")
	const TArray<FDiscoveredCookingRecipe>& GetDiscoveredRecipes() const { return DiscoveredRecipes; }

	UFUNCTION(BlueprintPure, Category = "Cooking Recipe Book")
	bool HasDiscoveredRecipe(FName RecipeKey) const;

	UFUNCTION(BlueprintPure, Category = "Cooking Recipe Book")
	FName MakeRecipeKeyFromResult(const FCookingResultData& CookingResult) const;

	void RestoreRecipesForTravel(const TArray<FDiscoveredCookingRecipe>& SavedRecipes);

	UPROPERTY(BlueprintAssignable, Category = "Cooking Recipe Book")
	FOnCookingRecipeDiscovered OnRecipeDiscovered;

	UPROPERTY(BlueprintAssignable, Category = "Cooking Recipe Book")
	FOnCookingRecipeUpdated OnRecipeUpdated;

private:
	static bool IsBetterRecipeRecord(const FDiscoveredCookingRecipe& CurrentRecipe, const FCookingResultData& CookingResult);
	static void ApplyBestRecord(FDiscoveredCookingRecipe& Recipe, const FCookingResultData& CookingResult, UTexture2D* ResultIcon);

private:
	UPROPERTY(VisibleAnywhere, Category = "Cooking Recipe Book")
	TArray<FDiscoveredCookingRecipe> DiscoveredRecipes;
};
