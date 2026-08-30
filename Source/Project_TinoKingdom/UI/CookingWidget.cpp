// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Project_TinoKingdom/Component/CookingComponent.h"
#include "Project_TinoKingdom/Player/TinoPlayerController.h"

void UCookingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_StartCooking != nullptr)
	{
		Button_StartCooking->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleStartCookingClicked);
	}

	if (WidgetTree == nullptr)
	{
		return;
	}

	UVerticalBox* RootVerticalBox = WidgetTree->FindWidget<UVerticalBox>(TEXT("VerticalBox_79"));
	if (RootVerticalBox == nullptr)
	{
		return;
	}

	Button_CloseCooking = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_CloseCooking"));
	UTextBlock* CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("TextBlock_CloseCooking")
	);
	if (Button_CloseCooking != nullptr && CloseButtonText != nullptr)
	{
		CloseButtonText->SetText(FText::FromString(TEXT("끝내기")));
		Button_CloseCooking->AddChild(CloseButtonText);
		Button_CloseCooking->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleCloseCookingClicked);
		RootVerticalBox->AddChildToVerticalBox(Button_CloseCooking);
	}

	IngredientListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IngredientListBox"));
	RootVerticalBox->AddChildToVerticalBox(IngredientListBox);

	IngredientOptionButtons.Empty();
	IngredientOptionTexts.Empty();

	for (int32 Index = 0; Index < MaxIngredientOptionCount; ++Index)
	{
		UButton* OptionButton = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			*FString::Printf(TEXT("IngredientOptionButton_%d"), Index)
		);
		UTextBlock* OptionText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("IngredientOptionText_%d"), Index)
		);

		if (OptionButton == nullptr || OptionText == nullptr)
		{
			continue;
		}

		OptionText->SetText(FText::GetEmpty());
		OptionButton->AddChild(OptionText);
		IngredientListBox->AddChildToVerticalBox(OptionButton);

		IngredientOptionButtons.Add(OptionButton);
		IngredientOptionTexts.Add(OptionText);
	}

	if (IngredientOptionButtons.IsValidIndex(0))
	{
		IngredientOptionButtons[0]->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleIngredientOption0Clicked);
	}
	if (IngredientOptionButtons.IsValidIndex(1))
	{
		IngredientOptionButtons[1]->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleIngredientOption1Clicked);
	}
	if (IngredientOptionButtons.IsValidIndex(2))
	{
		IngredientOptionButtons[2]->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleIngredientOption2Clicked);
	}
	if (IngredientOptionButtons.IsValidIndex(3))
	{
		IngredientOptionButtons[3]->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleIngredientOption3Clicked);
	}
	if (IngredientOptionButtons.IsValidIndex(4))
	{
		IngredientOptionButtons[4]->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleIngredientOption4Clicked);
	}
	if (IngredientOptionButtons.IsValidIndex(5))
	{
		IngredientOptionButtons[5]->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleIngredientOption5Clicked);
	}
	if (IngredientOptionButtons.IsValidIndex(6))
	{
		IngredientOptionButtons[6]->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleIngredientOption6Clicked);
	}
	if (IngredientOptionButtons.IsValidIndex(7))
	{
		IngredientOptionButtons[7]->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleIngredientOption7Clicked);
	}

	SetIngredientListVisible(false);
}

void UCookingWidget::InitializeCookingWidget(
	UCookingComponent* InCookingComponent,
	UInventoryComponent* InInventoryComponent
)
{
	CookingComponent = InCookingComponent;
	InventoryComponent = InInventoryComponent;

	RefreshIngredientList();
	SetIngredientListVisible(false);
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

	if (!CookingComponent->GetSelectedIngredients().IsValidIndex(Index))
	{
		ToggleIngredientList();
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
		SetIngredientListVisible(false);
		return;
	}

	SetResultText(FText::FromString(TEXT("재료가 부족합니다")));
}

void UCookingWidget::ToggleIngredientList()
{
	SetIngredientListVisible(!bIngredientListVisible);
}

void UCookingWidget::SetIngredientListVisible(bool bVisible)
{
	bIngredientListVisible = bVisible;

	if (IngredientListBox != nullptr)
	{
		IngredientListBox->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (bVisible)
	{
		RefreshIngredientList();
	}
}

void UCookingWidget::RefreshIngredientList()
{
	IngredientOptions.Empty();

	if (InventoryComponent != nullptr)
	{
		for (const FInventoryItemStack& Item : InventoryComponent->GetItems())
		{
			if (Item.Count <= 0 || Item.ItemType != EInventoryItemType::Material)
			{
				continue;
			}

			IngredientOptions.Add(Item);
			if (IngredientOptions.Num() >= MaxIngredientOptionCount)
			{
				break;
			}
		}
	}

	for (int32 Index = 0; Index < IngredientOptionButtons.Num(); ++Index)
	{
		UButton* OptionButton = IngredientOptionButtons[Index].Get();
		UTextBlock* OptionText = IngredientOptionTexts.IsValidIndex(Index) ? IngredientOptionTexts[Index].Get() : nullptr;

		if (OptionButton == nullptr || OptionText == nullptr)
		{
			continue;
		}

		if (IngredientOptions.IsValidIndex(Index))
		{
			const FInventoryItemStack& Item = IngredientOptions[Index];
			OptionText->SetText(FText::Format(
				FText::FromString(TEXT("{0} x{1}")),
				Item.DisplayName,
				FText::AsNumber(Item.Count)
			));
			OptionButton->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			OptionText->SetText(FText::GetEmpty());
			OptionButton->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (bIngredientListVisible && IngredientOptions.Num() == 0)
	{
		SetResultText(FText::FromString(TEXT("요리 재료가 없습니다")));
	}
}

void UCookingWidget::SelectIngredientOption(int32 OptionIndex)
{
	if (!IngredientOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	if (AddIngredientFromInventory(IngredientOptions[OptionIndex]))
	{
		SetResultText(FText::GetEmpty());
		SetIngredientListVisible(false);
	}
}

void UCookingWidget::HandleIngredientOption0Clicked()
{
	SelectIngredientOption(0);
}

void UCookingWidget::HandleIngredientOption1Clicked()
{
	SelectIngredientOption(1);
}

void UCookingWidget::HandleIngredientOption2Clicked()
{
	SelectIngredientOption(2);
}

void UCookingWidget::HandleIngredientOption3Clicked()
{
	SelectIngredientOption(3);
}

void UCookingWidget::HandleIngredientOption4Clicked()
{
	SelectIngredientOption(4);
}

void UCookingWidget::HandleIngredientOption5Clicked()
{
	SelectIngredientOption(5);
}

void UCookingWidget::HandleIngredientOption6Clicked()
{
	SelectIngredientOption(6);
}

void UCookingWidget::HandleIngredientOption7Clicked()
{
	SelectIngredientOption(7);
}

void UCookingWidget::HandleCloseCookingClicked()
{
	CloseCookingWidget();
}

void UCookingWidget::CloseCookingWidget()
{
	if (ATinoPlayerController* TinoPlayerController = Cast<ATinoPlayerController>(GetOwningPlayer()))
	{
		TinoPlayerController->ToggleCookingMenu(nullptr, nullptr);
		return;
	}

	RemoveFromParent();
}
