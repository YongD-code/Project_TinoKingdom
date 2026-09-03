// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ContentWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Project_TinoKingdom/Component/CookingComponent.h"
#include "Project_TinoKingdom/Player/TinoPlayerController.h"
#include "Project_TinoKingdom/UI/CookingMinigameWidget.h"

namespace
{
ECookingTag InferCookingTagFromItem(const FInventoryItemStack& Item)
{
	if (Item.CookingTag != ECookingTag::None)
	{
		return Item.CookingTag;
	}

	const FString SearchText = Item.ItemId.ToString() + TEXT(" ") + Item.DisplayName.ToString();

	if (SearchText.Contains(TEXT("Slime")) || SearchText.Contains(TEXT("슬라임")))
	{
		return ECookingTag::Slime;
	}
	if (SearchText.Contains(TEXT("WaterBest")) || SearchText.Contains(TEXT("Fish")) || SearchText.Contains(TEXT("Fin")) ||
		SearchText.Contains(TEXT("물짱")) || SearchText.Contains(TEXT("생선")) || SearchText.Contains(TEXT("지느러미")))
	{
		return ECookingTag::Fish;
	}
	if (SearchText.Contains(TEXT("Mushroom")) || SearchText.Contains(TEXT("버섯")))
	{
		return ECookingTag::Mushroom;
	}
	if (SearchText.Contains(TEXT("Meat")) || SearchText.Contains(TEXT("고기")))
	{
		return ECookingTag::Meat;
	}
	if (SearchText.Contains(TEXT("Herb")) || SearchText.Contains(TEXT("약초")))
	{
		return ECookingTag::Herb;
	}
	if (SearchText.Contains(TEXT("Wood")) || SearchText.Contains(TEXT("나무")))
	{
		return ECookingTag::Wood;
	}

	return ECookingTag::None;
}

bool MakeCookingIngredient(const FInventoryItemStack& SourceItem, FInventoryItemStack& OutIngredient)
{
	const ECookingTag CookingTag = InferCookingTagFromItem(SourceItem);
	if (CookingTag == ECookingTag::None || SourceItem.Count <= 0)
	{
		return false;
	}

	OutIngredient = SourceItem;
	OutIngredient.ItemType = EInventoryItemType::Material;
	OutIngredient.CookingTag = CookingTag;
	OutIngredient.Count = 1;
	return true;
}
}

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

	NormalizeCookingWidgetLayering();

	CloseCookingButton = WidgetTree->FindWidget<UButton>(TEXT("Button_CloseCooking_Static"));
	if (CloseCookingButton != nullptr)
	{
		CloseCookingButton->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleCloseCookingClicked);
	}
	if (UTextBlock* CloseButtonText = WidgetTree->FindWidget<UTextBlock>(TEXT("TextBlock_CloseCooking_Static")))
	{
		CloseButtonText->SetText(FText::FromString(TEXT("끝내기")));
	}

	CacheIngredientSlotButtonStyles();
}

void UCookingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CookingComponent != nullptr)
	{
		ResetCookingSelection();
	}
}

void UCookingWidget::NormalizeCookingWidgetLayering()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	WidgetTree->ForEachWidget([](UWidget* Widget)
	{
		if (UImage* ImageWidget = Cast<UImage>(Widget))
		{
			const UObject* BrushResource = ImageWidget->GetBrush().GetResourceObject();
			if (BrushResource != nullptr && BrushResource->GetName().Contains(TEXT("CookingPanelBackground_Medieval")))
			{
				ImageWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ImageWidget->Slot))
				{
					CanvasSlot->SetZOrder(0);
				}
			}
		}
	});
}

void UCookingWidget::NativeDestruct()
{
	if (ActiveMinigameWidget != nullptr)
	{
		ActiveMinigameWidget->OnCookingMinigameFinished.RemoveDynamic(this, &UCookingWidget::HandleCookingMinigameFinished);
		ActiveMinigameWidget->RemoveFromParent();
		ActiveMinigameWidget = nullptr;
	}

	Super::NativeDestruct();
}

void UCookingWidget::InitializeCookingWidget(
	UCookingComponent* InCookingComponent,
	UInventoryComponent* InInventoryComponent
)
{
	CookingComponent = InCookingComponent;
	InventoryComponent = InInventoryComponent;

	if (CookingComponent != nullptr)
	{
		ResetCookingSelection();
	}

	RefreshCookingActions();
}

void UCookingWidget::ResetCookingSelection()
{
	if (CookingComponent == nullptr)
	{
		ReservedIngredients.Empty();
		BroadcastSelectedIngredientsChanged();
		return;
	}

	RestoreReservedIngredients();
	CookingComponent->ClearCookingIngredients();
	BroadcastSelectedIngredientsChanged();
	RefreshLinkedInventoryPicker();
	SetResultText(FText::GetEmpty());
}

bool UCookingWidget::AddIngredientFromInventory(const FInventoryItemStack& Ingredient)
{
	if (CookingComponent == nullptr)
	{
		return false;
	}

	FInventoryItemStack CookingIngredient;
	if (!MakeCookingIngredient(Ingredient, CookingIngredient))
	{
		return false;
	}

	bool bRemovedFromInventory = false;
	if (InventoryComponent != nullptr)
	{
		if (!InventoryComponent->RemoveItem(CookingIngredient.ItemId, 1))
		{
			SetResultText(FText::FromString(TEXT("재료가 부족합니다")));
			return false;
		}

		bRemovedFromInventory = true;
	}

	const bool bAdded = CookingComponent->AddCookingIngredient(CookingIngredient);
	if (bAdded)
	{
		if (bRemovedFromInventory)
		{
			ReservedIngredients.Add(CookingIngredient);
		}

		SetResultText(FText::GetEmpty());
		BroadcastSelectedIngredientsChanged();
		RefreshLinkedInventoryPicker();
		return true;
	}

	if (bRemovedFromInventory && InventoryComponent != nullptr)
	{
		InventoryComponent->AddItem(
			CookingIngredient.ItemId,
			CookingIngredient.DisplayName,
			1,
			CookingIngredient.Icon,
			CookingIngredient.ItemType,
			CookingIngredient.CookingTag,
			CookingIngredient.FoodEffectType,
			CookingIngredient.CookingPower,
			CookingIngredient.FoodResultData
		);
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
		OpenIngredientPicker();
		return;
	}

	RestoreReservedIngredientAt(Index);
	CookingComponent->RemoveCookingIngredientAt(Index);
	BroadcastSelectedIngredientsChanged();
	RefreshLinkedInventoryPicker();
}

void UCookingWidget::ClearIngredients()
{
	if (CookingComponent == nullptr)
	{
		return;
	}

	ResetCookingSelection();
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
		OutResult,
		false
	);

	if (bCompleted)
	{
		ReservedIngredients.Empty();
		SetResultText(OutResult.ResultName);
		OnCookingCompleted(OutResult);
		BroadcastSelectedIngredientsChanged();
		RefreshLinkedInventoryPicker();
	}

	return bCompleted;
}

const TArray<FInventoryItemStack>& UCookingWidget::GetSelectedIngredients() const
{
	static const TArray<FInventoryItemStack> EmptyIngredients;
	return CookingComponent != nullptr ? CookingComponent->GetSelectedIngredients() : EmptyIngredients;
}

void UCookingWidget::BroadcastSelectedIngredientsChanged()
{
	if (CookingComponent == nullptr)
	{
		OnSelectedIngredientsChanged(TArray<FInventoryItemStack>());
		UpdateIngredientSlotTexts(TArray<FInventoryItemStack>());
		UpdateIngredientSlotImages(TArray<FInventoryItemStack>());
		return;
	}

	const TArray<FInventoryItemStack>& SelectedIngredients = CookingComponent->GetSelectedIngredients();
	OnSelectedIngredientsChanged(SelectedIngredients);
	UpdateIngredientSlotTexts(SelectedIngredients);
	UpdateIngredientSlotImages(SelectedIngredients);
	RefreshCookingActions();
	InvalidateLayoutAndVolatility();
}

void UCookingWidget::UpdateIngredientSlotTexts(const TArray<FInventoryItemStack>& SelectedIngredients)
{
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (SelectedIngredients.IsValidIndex(Index))
		{
			SetIngredientSlotText(Index, FText::GetEmpty());
		}
		else
		{
			SetIngredientSlotText(Index, FText::FromString(TEXT("+")));
		}
	}
}

void UCookingWidget::UpdateIngredientSlotImages(const TArray<FInventoryItemStack>& SelectedIngredients)
{
	for (int32 Index = 0; Index < 4; ++Index)
	{
		SetIngredientSlotImage(Index, SelectedIngredients.IsValidIndex(Index) ? SelectedIngredients[Index].Icon : nullptr);
	}
}

void UCookingWidget::SetIngredientSlotText(int32 Index, const FText& Text)
{
	if (UTextBlock* SlotTextBlock = FindIngredientSlotTextBlock(Index))
	{
		SlotTextBlock->SetText(Text);
		SlotTextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UCookingWidget::SetIngredientSlotImage(int32 Index, UTexture2D* Icon)
{
	UButton* SlotButton = FindIngredientSlotButton(Index);
	if (SlotButton == nullptr)
	{
		return;
	}

	if (Icon == nullptr)
	{
		if (IngredientSlotButtonStyles.IsValidIndex(Index))
		{
			SlotButton->SetStyle(IngredientSlotButtonStyles[Index]);
		}

		if (UTextBlock* SlotTextBlock = FindIngredientSlotTextBlock(Index))
		{
			SlotButton->SetContent(SlotTextBlock);
		}
		return;
	}

	if (IngredientSlotButtonStyles.IsValidIndex(Index))
	{
		SlotButton->SetStyle(IngredientSlotButtonStyles[Index]);
	}

	UImage* SlotImage = nullptr;
	if (IngredientSlotImages.IsValidIndex(Index))
	{
		SlotImage = IngredientSlotImages[Index].Get();
	}

	if (SlotImage == nullptr && WidgetTree != nullptr)
	{
		const FName RuntimeImageName(*FString::Printf(TEXT("CookingIngredientSlotImage_%d"), Index));
		SlotImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), RuntimeImageName);
		if (IngredientSlotImages.IsValidIndex(Index))
		{
			IngredientSlotImages[Index] = SlotImage;
		}
	}

	if (SlotImage != nullptr)
	{
		SlotImage->SetBrushFromTexture(Icon, true);
		SlotImage->SetDesiredSizeOverride(FVector2D(86.0f, 86.0f));
		SlotImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		SlotButton->SetContent(SlotImage);
	}
}

void UCookingWidget::CacheIngredientSlotButtonStyles()
{
	IngredientSlotButtonStyles.Reset();
	IngredientSlotButtonStyles.Reserve(4);
	IngredientSlotTextBlocks.Reset();
	IngredientSlotTextBlocks.Reserve(4);
	IngredientSlotImages.Reset();
	IngredientSlotImages.SetNum(4);

	for (int32 Index = 0; Index < 4; ++Index)
	{
		IngredientSlotTextBlocks.Add(FindIngredientSlotTextBlock(Index));

		if (UButton* SlotButton = FindIngredientSlotButton(Index))
		{
			IngredientSlotButtonStyles.Add(SlotButton->GetStyle());
			continue;
		}

		IngredientSlotButtonStyles.Add(FButtonStyle());
	}
}

UButton* UCookingWidget::FindIngredientSlotButton(int32 Index) const
{
	static const FName SlotButtonNames[] =
	{
		TEXT("M1"),
		TEXT("M2"),
		TEXT("M3"),
		TEXT("M4")
	};

	if (Index < 0 || Index >= UE_ARRAY_COUNT(SlotButtonNames) || WidgetTree == nullptr)
	{
		return nullptr;
	}

	return WidgetTree->FindWidget<UButton>(SlotButtonNames[Index]);
}

UTextBlock* UCookingWidget::FindIngredientSlotTextBlock(int32 Index) const
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
		return nullptr;
	}

	if (IngredientSlotTextBlocks.IsValidIndex(Index) && IngredientSlotTextBlocks[Index] != nullptr)
	{
		return IngredientSlotTextBlocks[Index].Get();
	}

	return WidgetTree->FindWidget<UTextBlock>(SlotTextBlockNames[Index]);
}

void UCookingWidget::RestoreReservedIngredientAt(int32 Index)
{
	if (!ReservedIngredients.IsValidIndex(Index))
	{
		return;
	}

	if (InventoryComponent != nullptr)
	{
		const FInventoryItemStack& Ingredient = ReservedIngredients[Index];
		InventoryComponent->AddItem(
			Ingredient.ItemId,
			Ingredient.DisplayName,
			1,
			Ingredient.Icon,
			Ingredient.ItemType,
			Ingredient.CookingTag,
			Ingredient.FoodEffectType,
			Ingredient.CookingPower,
			Ingredient.FoodResultData
		);
	}

	ReservedIngredients.RemoveAt(Index);
}

void UCookingWidget::RestoreReservedIngredients()
{
	if (InventoryComponent != nullptr)
	{
		for (const FInventoryItemStack& Ingredient : ReservedIngredients)
		{
			InventoryComponent->AddItem(
				Ingredient.ItemId,
				Ingredient.DisplayName,
				1,
				Ingredient.Icon,
				Ingredient.ItemType,
				Ingredient.CookingTag,
				Ingredient.FoodEffectType,
				Ingredient.CookingPower,
				Ingredient.FoodResultData
			);
		}
	}

	ReservedIngredients.Empty();
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
	if (!CanStartCooking())
	{
		SetResultText(FText::FromString(TEXT("재료가 부족합니다")));
		return;
	}

	OpenCookingMinigame();
}

void UCookingWidget::HandleCookingMinigameFinished(float FinalScore)
{
	if (ActiveMinigameWidget != nullptr)
	{
		ActiveMinigameWidget->OnCookingMinigameFinished.RemoveDynamic(this, &UCookingWidget::HandleCookingMinigameFinished);
		ActiveMinigameWidget->RemoveFromParent();
		ActiveMinigameWidget = nullptr;
	}

	FCookingResultData Result;
	if (CompleteCooking(FinalScore, Result))
	{
		return;
	}

	SetResultText(FText::FromString(TEXT("재료가 부족합니다")));
}

void UCookingWidget::OpenCookingMinigame()
{
	if (ActiveMinigameWidget != nullptr)
	{
		return;
	}

	TSubclassOf<UCookingMinigameWidget> MinigameClass = CookingMinigameWidgetClass;
	if (MinigameClass == nullptr)
	{
		MinigameClass = UCookingMinigameWidget::StaticClass();
	}

	ActiveMinigameWidget = CreateWidget<UCookingMinigameWidget>(GetOwningPlayer(), MinigameClass);
	if (ActiveMinigameWidget == nullptr)
	{
		FCookingResultData Result;
		if (!CompleteCooking(DefaultCookingScore, Result))
		{
			SetResultText(FText::FromString(TEXT("재료가 부족합니다")));
		}
		return;
	}

	ActiveMinigameWidget->OnCookingMinigameFinished.AddUniqueDynamic(this, &UCookingWidget::HandleCookingMinigameFinished);
	ActiveMinigameWidget->AddToViewport(30);
	ActiveMinigameWidget->StartCookingMinigame();
	ActiveMinigameWidget->SetKeyboardFocus();
}

bool UCookingWidget::CanStartCooking() const
{
	return CookingComponent != nullptr && CookingComponent->CanFinishCooking();
}

void UCookingWidget::RefreshCookingActions()
{
	if (Button_StartCooking != nullptr)
	{
		Button_StartCooking->SetIsEnabled(CanStartCooking());
	}
}

void UCookingWidget::RefreshLinkedInventoryPicker()
{
	if (ATinoPlayerController* TinoPlayerController = Cast<ATinoPlayerController>(GetOwningPlayer()))
	{
		TinoPlayerController->RefreshCookingIngredientPicker();
	}
}

void UCookingWidget::OpenIngredientPicker()
{
	if (ATinoPlayerController* TinoPlayerController = Cast<ATinoPlayerController>(GetOwningPlayer()))
	{
		TinoPlayerController->ShowCookingIngredientPicker(this, InventoryComponent);
	}
}

void UCookingWidget::HandleCloseCookingClicked()
{
	CloseCookingWidget();
}

void UCookingWidget::CloseCookingWidget()
{
	if (ActiveMinigameWidget != nullptr)
	{
		ActiveMinigameWidget->OnCookingMinigameFinished.RemoveDynamic(this, &UCookingWidget::HandleCookingMinigameFinished);
		ActiveMinigameWidget->RemoveFromParent();
		ActiveMinigameWidget = nullptr;
	}

	if (CookingComponent != nullptr)
	{
		ResetCookingSelection();
	}

	if (ATinoPlayerController* TinoPlayerController = Cast<ATinoPlayerController>(GetOwningPlayer()))
	{
		TinoPlayerController->ToggleCookingMenu(nullptr, nullptr);
		return;
	}

	RemoveFromParent();
}
