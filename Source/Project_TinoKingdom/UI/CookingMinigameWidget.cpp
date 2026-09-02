// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingMinigameWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"

void UCookingMinigameWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetIsFocusable(true);
	BuildDefaultMinigameVisuals();
}

void UCookingMinigameWidget::StartCookingMinigame()
{
	bMinigamePlaying = true;
	bCookingInputHeld = false;
	TargetDirection = FMath::RandBool() ? 1.0f : -1.0f;
	ResetTargetDirectionTimer();

	ElapsedTime = 0.0f;
	MinigameScore = 50.0f;
	PlayerBarPosition = 0.5f;
	TargetPosition = 0.5f;

	OnMinigameUpdated(PlayerBarPosition, TargetPosition, MinigameScore, GetRemainingTime());
	RefreshDefaultMinigameVisuals();
}

void UCookingMinigameWidget::SetCookingInputHeld(bool bHeld)
{
	bCookingInputHeld = bHeld;
}

void UCookingMinigameWidget::FinishCookingMinigame()
{
	if (!bMinigamePlaying)
	{
		return;
	}

	bMinigamePlaying = false;

	OnCookingMinigameFinished.Broadcast(MinigameScore);
	OnMinigameEnded(MinigameScore);
	RefreshDefaultMinigameVisuals();
}

float UCookingMinigameWidget::GetRemainingTime() const
{
	return FMath::Max(Duration - ElapsedTime, 0.0f);
}

void UCookingMinigameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bMinigamePlaying)
	{
		return;
	}

	ElapsedTime += InDeltaTime;

	TargetDirectionChangeTimer -= InDeltaTime;
	if (TargetDirectionChangeTimer <= 0.0f)
	{
		TargetDirection = FMath::RandBool() ? 1.0f : -1.0f;
		ResetTargetDirectionTimer();
	}

	TargetPosition += TargetDirection * TargetMoveSpeed * InDeltaTime;

	if (TargetPosition >= 1.0f)
	{
		TargetPosition = 1.0f;
		TargetDirection = -1.0f;
		ResetTargetDirectionTimer();
	}
	else if (TargetPosition <= 0.0f)
	{
		TargetPosition = 0.0f;
		TargetDirection = 1.0f;
		ResetTargetDirectionTimer();
	}

	const float PlayerDirection = bCookingInputHeld ? 1.0f : -1.0f;
	const float PlayerSpeed = bCookingInputHeld ? PlayerBarRiseSpeed : PlayerBarFallSpeed;
	PlayerBarPosition = FMath::Clamp(PlayerBarPosition + PlayerDirection * PlayerSpeed * InDeltaTime, 0.0f, 1.0f);

	const float ScoreDelta = IsPlayerBarInTargetZone() ? ScoreIncreasePerSecond : -ScoreDecreasePerSecond;
	MinigameScore = FMath::Clamp(MinigameScore + ScoreDelta * InDeltaTime, 0.0f, 100.0f);

	OnMinigameUpdated(PlayerBarPosition, TargetPosition, MinigameScore, GetRemainingTime());
	RefreshDefaultMinigameVisuals();

	if (ElapsedTime >= Duration)
	{
		FinishCookingMinigame();
	}
}

FReply UCookingMinigameWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SetCookingInputHeld(true);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCookingMinigameWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SetCookingInputHeld(false);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UCookingMinigameWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		SetCookingInputHeld(true);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UCookingMinigameWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		SetCookingInputHeld(false);
		return FReply::Handled();
	}

	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

void UCookingMinigameWidget::BuildDefaultMinigameVisuals()
{
	if (WidgetTree == nullptr || GetRootWidget() != nullptr)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CookingMinigameCanvas_Runtime"));
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CookingMinigameRoot_Runtime"));
	USizeBox* PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CookingMinigameSize_Runtime"));
	UVerticalBox* LayoutBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CookingMinigameLayout_Runtime"));

	if (RootCanvas == nullptr || RootBorder == nullptr || PanelSizeBox == nullptr || LayoutBox == nullptr)
	{
		return;
	}

	InstructionTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigameInstruction_Runtime"));
	ScoreTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigameScore_Runtime"));
	TargetLabelTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigameTargetLabel_Runtime"));
	PlayerLabelTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigamePlayerLabel_Runtime"));
	TimeLabelTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigameTimeLabel_Runtime"));
	TargetProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CookingMinigameTarget_Runtime"));
	PlayerProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CookingMinigamePlayer_Runtime"));
	TimeProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CookingMinigameTime_Runtime"));

	if (InstructionTextBlock == nullptr || ScoreTextBlock == nullptr || TargetLabelTextBlock == nullptr ||
		PlayerLabelTextBlock == nullptr || TimeLabelTextBlock == nullptr || TargetProgressBar == nullptr ||
		PlayerProgressBar == nullptr || TimeProgressBar == nullptr)
	{
		return;
	}

	RootCanvas->SetVisibility(ESlateVisibility::Visible);
	RootBorder->SetBrushColor(FLinearColor(0.018f, 0.018f, 0.022f, 0.94f));
	RootBorder->SetPadding(FMargin(28.0f));
	RootBorder->SetContent(LayoutBox);
	PanelSizeBox->SetWidthOverride(920.0f);
	PanelSizeBox->SetHeightOverride(360.0f);
	PanelSizeBox->SetContent(RootBorder);

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSizeBox);
	if (PanelSlot != nullptr)
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
		PanelSlot->SetSize(FVector2D(920.0f, 360.0f));
	}

	InstructionTextBlock->SetText(FText::FromString(TEXT("조리 타이밍")));
	InstructionTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.86f, 0.45f, 1.0f)));
	ScoreTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TargetLabelTextBlock->SetText(FText::FromString(TEXT("목표 바")));
	PlayerLabelTextBlock->SetText(FText::FromString(TEXT("플레이어 바")));
	TimeLabelTextBlock->SetText(FText::FromString(TEXT("남은 시간")));
	TargetLabelTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.72f, 0.20f, 1.0f)));
	PlayerLabelTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.85f, 1.0f, 1.0f)));
	TimeLabelTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 1.0f, 0.50f, 1.0f)));

	TargetProgressBar->SetFillColorAndOpacity(FLinearColor(1.0f, 0.66f, 0.16f, 1.0f));
	PlayerProgressBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.8f, 1.0f, 1.0f));
	TimeProgressBar->SetFillColorAndOpacity(FLinearColor(0.45f, 0.95f, 0.48f, 1.0f));

	LayoutBox->AddChildToVerticalBox(InstructionTextBlock);
	LayoutBox->AddChildToVerticalBox(ScoreTextBlock);
	LayoutBox->AddChildToVerticalBox(TargetLabelTextBlock);
	LayoutBox->AddChildToVerticalBox(TargetProgressBar);
	LayoutBox->AddChildToVerticalBox(PlayerLabelTextBlock);
	LayoutBox->AddChildToVerticalBox(PlayerProgressBar);
	LayoutBox->AddChildToVerticalBox(TimeLabelTextBlock);
	LayoutBox->AddChildToVerticalBox(TimeProgressBar);

	WidgetTree->RootWidget = RootCanvas;
	RefreshDefaultMinigameVisuals();
}

void UCookingMinigameWidget::RefreshDefaultMinigameVisuals()
{
	if (ScoreTextBlock != nullptr)
	{
		ScoreTextBlock->SetText(FText::FromString(FString::Printf(
			TEXT("목표를 따라가세요  점수 %d  남은 시간 %.1f"),
			FMath::RoundToInt(MinigameScore),
			GetRemainingTime()
		)));
	}

	if (TargetProgressBar != nullptr)
	{
		TargetProgressBar->SetPercent(TargetPosition);
	}
	if (PlayerProgressBar != nullptr)
	{
		PlayerProgressBar->SetPercent(PlayerBarPosition);
	}
	if (TimeProgressBar != nullptr)
	{
		TimeProgressBar->SetPercent(Duration > 0.0f ? GetRemainingTime() / Duration : 0.0f);
	}
}

void UCookingMinigameWidget::ResetTargetDirectionTimer()
{
	const float MinTime = FMath::Max(0.1f, TargetDirectionChangeMinTime);
	const float MaxTime = FMath::Max(MinTime, TargetDirectionChangeMaxTime);
	TargetDirectionChangeTimer = FMath::FRandRange(MinTime, MaxTime);
}

bool UCookingMinigameWidget::IsPlayerBarInTargetZone() const
{
	const float HalfTargetSize = TargetZoneSize * 0.5f;
	return FMath::Abs(PlayerBarPosition - TargetPosition) <= HalfTargetSize;
}
