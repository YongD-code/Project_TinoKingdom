// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Project_TinoKingdom/Component/CookingComponent.h"

void UCookingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_StartCooking != nullptr)
	{
		Button_StartCooking->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleStartCookingClicked);
	}
}

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

bool UCookingWidget::AddFirstAvailableIngredientFromInventory()
{
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	for (const FInventoryItemStack& Item : InventoryComponent->GetItems())
	{
		if (Item.Count <= 0 || Item.ItemType != EInventoryItemType::Material)
		{
			continue;
		}

		if (AddIngredientFromInventory(Item))
		{
			return true;
		}
	}

	return false;
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
		SetResultText(OutResult.ResultName);
		OnCookingCompleted(OutResult);
		BroadcastSelectedIngredientsChanged();
	}

	return bCompleted;
}

void UCookingWidget::BroadcastSelectedIngredientsChanged()
{
	if (CookingComponent == nullptr)
	{
		UpdateIngredientSlotTexts(TArray<FInventoryItemStack>());
		OnSelectedIngredientsChanged(TArray<FInventoryItemStack>());
		return;
	}

	const TArray<FInventoryItemStack>& SelectedIngredients = CookingComponent->GetSelectedIngredients();
	UpdateIngredientSlotTexts(SelectedIngredients);
	OnSelectedIngredientsChanged(SelectedIngredients);
}

void UCookingWidget::UpdateIngredientSlotTexts(const TArray<FInventoryItemStack>& SelectedIngredients)
{
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (SelectedIngredients.IsValidIndex(Index))
		{
			SetIngredientSlotText(Index, SelectedIngredients[Index].DisplayName);
		}
		else
		{
			SetIngredientSlotText(Index, FText::FromString(TEXT("+")));
		}
	}
}

void UCookingWidget::SetIngredientSlotText(int32 Index, const FText& Text)
{
	static const FName SlotTextBlockNames[] =
	{
		TEXT("TextBlock_0"),
		TEXT("TextBlock_1"),
		TEXT("TextBlock_2"),
		TEXT("TextBlock_3")
	};

	if (Index < 0 || Index >= UE_ARRAY_COUNT(SlotTextBlockNames) || WidgetTree == nullptr)
	{
		return;
	}

	if (UTextBlock* SlotTextBlock = WidgetTree->FindWidget<UTextBlock>(SlotTextBlockNames[Index]))
	{
		SlotTextBlock->SetText(Text);
	}
}

void UCookingWidget::SetResultText(const FText& Text)
{
	if (TextBox_Result != nullptr)
	{
		TextBox_Result->SetText(Text);
	}
}

void UCookingWidget::HandleStartCookingClicked()
{
	FCookingResultData Result;
	if (CompleteCooking(DefaultCookingScore, Result))
	{
		return;
	}

	SetResultText(FText::FromString(TEXT("재료가 부족합니다")));
}
