// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
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

	CloseCookingButton = WidgetTree->FindWidget<UButton>(TEXT("Button_CloseCooking_Static"));
	if (CloseCookingButton != nullptr)
	{
		CloseCookingButton->OnClicked.AddUniqueDynamic(this, &UCookingWidget::HandleCloseCookingClicked);
	}
	if (UTextBlock* CloseButtonText = WidgetTree->FindWidget<UTextBlock>(TEXT("TextBlock_CloseCooking_Static")))
	{
		CloseButtonText->SetText(FText::FromString(TEXT("끝내기")));
	}

	IngredientListBox = WidgetTree->FindWidget<UVerticalBox>(TEXT("IngredientInventoryPanel"));
	IngredientOptionButtons.Empty();
	IngredientOptionTexts.Empty();

	for (int32 Index = 0; Index < MaxIngredientOptionCount; ++Index)
	{
		const FName ButtonName(*FString::Printf(TEXT("IngredientOptionButton_%d"), Index));
		const FName TextName(*FString::Printf(TEXT("IngredientOptionText_%d"), Index));

		IngredientOptionButtons.Add(WidgetTree->FindWidget<UButton>(ButtonName));
		IngredientOptionTexts.Add(WidgetTree->FindWidget<UTextBlock>(TextName));
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

int32 UCookingWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	const FVector2D PanelPadding(14.0f, 12.0f);
	const FVector2D ContentMin = PanelPadding;
	const FVector2D ContentSize = WidgetSize - PanelPadding * 2.0f;

	const FLinearColor WoodDark(0.12f, 0.065f, 0.032f, 0.96f);
	const FLinearColor WoodMid(0.26f, 0.15f, 0.07f, 0.96f);
	const FLinearColor IronDark(0.05f, 0.045f, 0.04f, 0.98f);
	const FLinearColor Brass(0.86f, 0.58f, 0.22f, 1.0f);
	const FLinearColor Parchment(0.58f, 0.53f, 0.43f, 0.92f);
	const FLinearColor SlotFill(0.05f, 0.042f, 0.032f, 0.94f);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(WidgetSize, FSlateLayoutTransform(FVector2D::ZeroVector)),
		WhiteBrush,
		ESlateDrawEffect::None,
		WoodDark
	);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(ContentSize, FSlateLayoutTransform(ContentMin)),
		WhiteBrush,
		ESlateDrawEffect::None,
		WoodMid
	);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 2,
		AllottedGeometry.ToPaintGeometry(ContentSize - FVector2D(10.0f, 10.0f), FSlateLayoutTransform(ContentMin + FVector2D(5.0f, 5.0f))),
		WhiteBrush,
		ESlateDrawEffect::None,
		IronDark
	);

	const int32 MaxLayer = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId + 3,
		InWidgetStyle,
		bParentEnabled
	);

	const int32 OverlayLayer = MaxLayer + 1;
	const float HeaderHeight = 34.0f;
	const float SlotY = 46.0f;
	const float SlotHeight = 72.0f;
	const float ButtonHeight = 56.0f;
	const float ButtonGap = 8.0f;
	const float SlotGap = 8.0f;
	const float SlotAreaX = 18.0f;
	const float SlotAreaWidth = FMath::Max(120.0f, WidgetSize.X - SlotAreaX * 2.0f);
	const float SlotWidth = (SlotAreaWidth - SlotGap * 3.0f) / 4.0f;

	static const TArray<FInventoryItemStack> EmptyIngredients;
	const TArray<FInventoryItemStack>& SelectedIngredients =
		CookingComponent != nullptr ? CookingComponent->GetSelectedIngredients() : EmptyIngredients;
	FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18);
	FSlateFontInfo ButtonFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 17);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		OverlayLayer,
		AllottedGeometry.ToPaintGeometry(FVector2D(WidgetSize.X, HeaderHeight), FSlateLayoutTransform(FVector2D(0.0f, 8.0f))),
		FText::FromString(TEXT("요리")),
		TitleFont,
		ESlateDrawEffect::None,
		FLinearColor(1.0f, 0.88f, 0.18f, 1.0f)
	);

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector2D SlotPosition(SlotAreaX + Index * (SlotWidth + SlotGap), SlotY);
		const FVector2D SlotSize(SlotWidth, SlotHeight);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			OverlayLayer,
			AllottedGeometry.ToPaintGeometry(SlotSize, FSlateLayoutTransform(SlotPosition)),
			WhiteBrush,
			ESlateDrawEffect::None,
			Brass
		);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			OverlayLayer + 1,
			AllottedGeometry.ToPaintGeometry(SlotSize - FVector2D(8.0f, 8.0f), FSlateLayoutTransform(SlotPosition + FVector2D(4.0f, 4.0f))),
			WhiteBrush,
			ESlateDrawEffect::None,
			SlotFill
		);

		if (SelectedIngredients.IsValidIndex(Index) && SelectedIngredients[Index].Icon != nullptr)
		{
			const float IconSide = FMath::Min(SlotSize.X, SlotSize.Y) * 0.86f;
			const FVector2D IconSize(IconSide, IconSide);
			const FVector2D IconPosition = SlotPosition + (SlotSize - IconSize) * 0.5f;

			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(SelectedIngredients[Index].Icon);
			IconBrush.ImageSize = IconSize;
			IconBrush.DrawAs = ESlateBrushDrawType::Image;

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				OverlayLayer + 2,
				AllottedGeometry.ToPaintGeometry(IconSize, FSlateLayoutTransform(IconPosition)),
				&IconBrush,
				ESlateDrawEffect::None,
				FLinearColor::White
			);
		}
		else
		{
			FSlateDrawElement::MakeText(
				OutDrawElements,
				OverlayLayer + 2,
				AllottedGeometry.ToPaintGeometry(SlotSize, FSlateLayoutTransform(SlotPosition + FVector2D(SlotWidth * 0.43f, 16.0f))),
				FText::FromString(TEXT("+")),
				FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 34),
				ESlateDrawEffect::None,
				FLinearColor(0.95f, 0.84f, 0.45f, 1.0f)
			);
		}
	}

	const FVector2D ButtonSize(WidgetSize.X - 28.0f, ButtonHeight);
	const float ButtonX = 14.0f;
	const float ClearY = SlotY + SlotHeight + 12.0f;
	const float StartY = ClearY + ButtonHeight + ButtonGap;
	const float CloseY = StartY + ButtonHeight + ButtonGap;

	const struct FButtonVisual
	{
		float Y;
		FText Text;
		FLinearColor Color;
	} ButtonVisuals[] =
	{
		{ ClearY, FText::FromString(TEXT("비우기")), Parchment },
		{ StartY, FText::FromString(TEXT("조리 시작")), FLinearColor(0.34f, 0.31f, 0.25f, 0.95f) },
		{ CloseY, FText::FromString(TEXT("끝내기")), Parchment }
	};

	for (const FButtonVisual& Visual : ButtonVisuals)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			OverlayLayer,
			AllottedGeometry.ToPaintGeometry(ButtonSize, FSlateLayoutTransform(FVector2D(ButtonX, Visual.Y))),
			WhiteBrush,
			ESlateDrawEffect::None,
			Visual.Color
		);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			OverlayLayer + 1,
			AllottedGeometry.ToPaintGeometry(ButtonSize, FSlateLayoutTransform(FVector2D(ButtonX + ButtonSize.X * 0.43f, Visual.Y + 16.0f))),
			Visual.Text,
			ButtonFont,
			ESlateDrawEffect::None,
			FLinearColor::White
		);
	}

	return OverlayLayer + 3;
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
	RefreshCookingActions();
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

	if (InventoryComponent != nullptr)
	{
		const int32 AvailableCount = InventoryComponent->GetItemCount(CookingIngredient.ItemId);
		int32 AlreadySelectedCount = 0;

		for (const FInventoryItemStack& SelectedIngredient : CookingComponent->GetSelectedIngredients())
		{
			if (SelectedIngredient.ItemId == CookingIngredient.ItemId)
			{
				++AlreadySelectedCount;
			}
		}

		if (AlreadySelectedCount >= AvailableCount)
		{
			return false;
		}
	}

	const bool bAdded = CookingComponent->AddCookingIngredient(CookingIngredient);
	if (bAdded)
	{
		BroadcastSelectedIngredientsChanged();
		RefreshLinkedInventoryPicker();
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
		FInventoryItemStack CookingIngredient;
		if (MakeCookingIngredient(Item, CookingIngredient) && AddIngredientFromInventory(CookingIngredient))
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
	RefreshLinkedInventoryPicker();
}

void UCookingWidget::ClearIngredients()
{
	if (CookingComponent == nullptr)
	{
		return;
	}

	CookingComponent->ClearCookingIngredients();
	BroadcastSelectedIngredientsChanged();
	RefreshLinkedInventoryPicker();
	SetResultText(FText::GetEmpty());
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

void UCookingWidget::SetIngredientSlotImage(int32 Index, UTexture2D* Icon)
{
	if (Index < 0 || Index >= 4 || WidgetTree == nullptr)
	{
		return;
	}

	SetIngredientSlotButtonIcon(Index, Icon);

	const FName CandidateNames[][4] =
	{
		{ TEXT("Image_0"), TEXT("Image_1"), TEXT("Image_2"), TEXT("Image_3") },
		{ TEXT("ImageSlot_0"), TEXT("ImageSlot_1"), TEXT("ImageSlot_2"), TEXT("ImageSlot_3") },
		{ TEXT("IngredientImage_0"), TEXT("IngredientImage_1"), TEXT("IngredientImage_2"), TEXT("IngredientImage_3") },
		{ TEXT("IngredientSlotImage_0"), TEXT("IngredientSlotImage_1"), TEXT("IngredientSlotImage_2"), TEXT("IngredientSlotImage_3") }
	};

	for (const auto& CandidateSet : CandidateNames)
	{
		if (UImage* SlotImage = WidgetTree->FindWidget<UImage>(CandidateSet[Index]))
		{
			if (Icon != nullptr)
			{
				SlotImage->SetBrushFromTexture(Icon, true);
				SlotImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				SlotImage->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

}

void UCookingWidget::SetIngredientSlotButtonIcon(int32 Index, UTexture2D* Icon)
{
	if (Index < 0 || Index >= 4 || WidgetTree == nullptr)
	{
		return;
	}

	const FName CandidateNames[][4] =
	{
		{ TEXT("Button_0"), TEXT("Button_1"), TEXT("Button_2"), TEXT("Button_3") },
		{ TEXT("SlotButton_0"), TEXT("SlotButton_1"), TEXT("SlotButton_2"), TEXT("SlotButton_3") },
		{ TEXT("IngredientButton_0"), TEXT("IngredientButton_1"), TEXT("IngredientButton_2"), TEXT("IngredientButton_3") },
		{ TEXT("IngredientSlotButton_0"), TEXT("IngredientSlotButton_1"), TEXT("IngredientSlotButton_2"), TEXT("IngredientSlotButton_3") }
	};

	for (const auto& CandidateSet : CandidateNames)
	{
		const FName ButtonName = CandidateSet[Index];
		UButton* SlotButton = WidgetTree->FindWidget<UButton>(ButtonName);
		if (SlotButton == nullptr)
		{
			continue;
		}

		if (!OriginalIngredientSlotButtonStyles.Contains(ButtonName))
		{
			OriginalIngredientSlotButtonStyles.Add(ButtonName, SlotButton->GetStyle());
		}

		if (Icon == nullptr)
		{
			if (const FButtonStyle* OriginalStyle = OriginalIngredientSlotButtonStyles.Find(ButtonName))
			{
				SlotButton->SetStyle(*OriginalStyle);
			}
			continue;
		}

		FButtonStyle IconStyle = SlotButton->GetStyle();
		FSlateBrush IconBrush;
		IconBrush.SetResourceObject(Icon);
		IconBrush.ImageSize = FVector2D(82.0f, 82.0f);
		IconBrush.DrawAs = ESlateBrushDrawType::Image;

		IconStyle.SetNormal(IconBrush);
		IconStyle.SetHovered(IconBrush);
		IconStyle.SetPressed(IconBrush);
		SlotButton->SetStyle(IconStyle);
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
		SetIngredientListVisible(false);
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

void UCookingWidget::ToggleIngredientList()
{
	if (ATinoPlayerController* TinoPlayerController = Cast<ATinoPlayerController>(GetOwningPlayer()))
	{
		SetIngredientListVisible(false);
		TinoPlayerController->ShowCookingIngredientPicker(this, InventoryComponent);
		return;
	}

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
			FInventoryItemStack CookingIngredient;
			if (!MakeCookingIngredient(Item, CookingIngredient))
			{
				continue;
			}

			CookingIngredient.Count = Item.Count;
			IngredientOptions.Add(CookingIngredient);
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
	if (ActiveMinigameWidget != nullptr)
	{
		ActiveMinigameWidget->OnCookingMinigameFinished.RemoveDynamic(this, &UCookingWidget::HandleCookingMinigameFinished);
		ActiveMinigameWidget->RemoveFromParent();
		ActiveMinigameWidget = nullptr;
	}

	if (ATinoPlayerController* TinoPlayerController = Cast<ATinoPlayerController>(GetOwningPlayer()))
	{
		TinoPlayerController->ToggleCookingMenu(nullptr, nullptr);
		return;
	}

	RemoveFromParent();
}
