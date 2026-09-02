// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingMinigameWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
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
	bTargetMovingUp = true;

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

	const float TargetDirection = bTargetMovingUp ? 1.0f : -1.0f;
	TargetPosition += TargetDirection * TargetMoveSpeed * InDeltaTime;

	if (TargetPosition >= 1.0f)
	{
		TargetPosition = 1.0f;
		bTargetMovingUp = false;
	}
	else if (TargetPosition <= 0.0f)
	{
		TargetPosition = 0.0f;
		bTargetMovingUp = true;
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

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CookingMinigameRoot_Runtime"));
	UVerticalBox* LayoutBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CookingMinigameLayout_Runtime"));

	if (RootBorder == nullptr || LayoutBox == nullptr)
	{
		return;
	}

	InstructionTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigameInstruction_Runtime"));
	ScoreTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigameScore_Runtime"));
	TargetProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CookingMinigameTarget_Runtime"));
	PlayerProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CookingMinigamePlayer_Runtime"));
	TimeProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CookingMinigameTime_Runtime"));

	if (InstructionTextBlock == nullptr || ScoreTextBlock == nullptr || TargetProgressBar == nullptr ||
		PlayerProgressBar == nullptr || TimeProgressBar == nullptr)
	{
		return;
	}

	RootBorder->SetBrushColor(FLinearColor(0.025f, 0.022f, 0.018f, 0.92f));
	RootBorder->SetPadding(FMargin(28.0f));
	RootBorder->SetContent(LayoutBox);

	InstructionTextBlock->SetText(FText::FromString(TEXT("조리 타이밍")));
	InstructionTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.86f, 0.45f, 1.0f)));
	ScoreTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	TargetProgressBar->SetFillColorAndOpacity(FLinearColor(1.0f, 0.66f, 0.16f, 1.0f));
	PlayerProgressBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.8f, 1.0f, 1.0f));
	TimeProgressBar->SetFillColorAndOpacity(FLinearColor(0.45f, 0.95f, 0.48f, 1.0f));

	LayoutBox->AddChildToVerticalBox(InstructionTextBlock);
	LayoutBox->AddChildToVerticalBox(ScoreTextBlock);
	LayoutBox->AddChildToVerticalBox(TargetProgressBar);
	LayoutBox->AddChildToVerticalBox(PlayerProgressBar);
	LayoutBox->AddChildToVerticalBox(TimeProgressBar);

	WidgetTree->RootWidget = RootBorder;
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

bool UCookingMinigameWidget::IsPlayerBarInTargetZone() const
{
	const float HalfTargetSize = TargetZoneSize * 0.5f;
	return FMath::Abs(PlayerBarPosition - TargetPosition) <= HalfTargetSize;
}
