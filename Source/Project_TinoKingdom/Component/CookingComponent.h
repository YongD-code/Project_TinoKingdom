// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Project_TinoKingdom/Types/CookingTypes.h"
#include "CookingComponent.generated.h"

struct FInventoryItemStack;

UCLASS(ClassGroup=(Tino), meta=(BlueprintSpawnableComponent))
class PROJECT_TINOKINGDOM_API UCookingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCookingComponent();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	FCookingResultData MakeCookingResult(const TArray<FInventoryItemStack>& Ingredients) const;

	UFUNCTION(BlueprintCallable, Category="Cooking")
	bool AddCookingIngredient(const FInventoryItemStack& Ingredient);

	UFUNCTION(BlueprintCallable, Category="Cooking")
	void RemoveCookingIngredientAt(int32 Index);

	UFUNCTION(BlueprintCallable, Category="Cooking")
	void ClearCookingIngredients();

	UFUNCTION(BlueprintPure, Category="Cooking")
	const TArray<FInventoryItemStack>& GetSelectedIngredients() const;
	
	UFUNCTION(BlueprintPure, Category = "Cooking")
	ECookingQuality GetQualityByMinigameScore(float MinigameScore) const;
	
	
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(BlueprintReadOnly, Category="Cooking", meta=(AllowPrivateAccess="true"))
	TArray<FInventoryItemStack> SelectedIngredients;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cooking", meta=(AllowPrivateAccess="true"))
	int32 MaxIngredientCount = 4;
	
	ECookingTag FindMainTag(const TMap<ECookingTag, float>& TagRatios) const;
	ECookingTag FindSubTag(const TMap<ECookingTag, float>& TagRatios, ECookingTag MainTag) const;

	ECookingResultType GetResultTypeByMainTag(ECookingTag MainTag) const;
	FLinearColor GetBaseTintByMainTag(ECookingTag MainTag) const;

	ECookingQuality CalculateQuality(
		const TArray<FInventoryItemStack>& Ingredients,
		const TMap<ECookingTag, float>& TagRatios
	) const;

	FText MakeResultName(
		ECookingTag MainTag,
		ECookingTag SubTag,
		ECookingResultType ResultType,
		ECookingQuality Quality
	) const;

	FName MakeResultItemId(
		ECookingTag MainTag,
		ECookingTag SubTag,
		ECookingResultType ResultType,
		ECookingQuality Quality
	) const;

	float GetQualityMultiplier(ECookingQuality Quality) const;
	FText GetTagDisplayName(ECookingTag Tag) const;
	FText GetResultTypeDisplayName(ECookingResultType ResultType) const;
};

