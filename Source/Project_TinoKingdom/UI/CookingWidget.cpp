// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingWidget.h"

#include "Project_TinoKingdom/Component/CookingComponent.h"

void UCookingWidget::InitializeCookingWidget(
	UCookingComponent* InCookingComponent,
	UInventoryComponent* InInventoryComponent
)
{
	CookingComponent = InCookingComponent;
	InventoryComponent = InInventoryComponent;

	BroadcastSelectedIngredientsChanged();
}

bool UCookingWidget::AddIngredientFromInventory(const FInventoryItemStack& Ingredient)
{
	if (CookingComponent == nullptr)
	{
		return false;
	}

	const bool bAdded = CookingComponent->AddCookingIngredient(Ingredient);
	if (bAdded)
	{
		BroadcastSelectedIngredientsChanged();
	}

	return bAdded;
}

void UCookingWidget::RemoveIngredientAt(int32 Index)
{
	if (CookingComponent == nullptr)
	{
		return;
	}

	CookingComponent->RemoveCookingIngredientAt(Index);
	BroadcastSelectedIngredientsChanged();
}

void UCookingWidget::ClearIngredients()
{
	if (CookingComponent == nullptr)
	{
		return;
	}

	CookingComponent->ClearCookingIngredients();
	BroadcastSelectedIngredientsChanged();
}

bool UCookingWidget::CompleteCooking(float MinigameScore, FCookingResultData& OutResult)
{
	if (CookingComponent == nullptr || InventoryComponent == nullptr)
	{
		return false;
	}

	const bool bCompleted = CookingComponent->FinishCookingToInventory(
		InventoryComponent,
		MinigameScore,
		OutResult
	);

	if (bCompleted)
	{
		OnCookingCompleted(OutResult);
		BroadcastSelectedIngredientsChanged();
	}

	return bCompleted;
}

void UCookingWidget::BroadcastSelectedIngredientsChanged()
{
	if (CookingComponent == nullptr)
	{
		OnSelectedIngredientsChanged(TArray<FInventoryItemStack>());
		return;
	}

	OnSelectedIngredientsChanged(CookingComponent->GetSelectedIngredients());
}
