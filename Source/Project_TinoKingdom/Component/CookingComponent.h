// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/Types/CookingTypes.h"
#include "CookingComponent.generated.h"

UCLASS(ClassGroup=(Tino), meta=(BlueprintSpawnableComponent))
class PROJECT_TINOKINGDOM_API UCookingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCookingComponent();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	FCookingResultData MakeCookingResult(const TArray<FInventoryItemStack>& Ingredients) const;

protected:
	virtual void BeginPlay() override;

private:
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