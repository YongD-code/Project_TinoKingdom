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

namespace
{
FString GetCookingMinigameQualityText(float Score)
{
	if (Score < 40.0f)
	{
		return TEXT("FAILED");
	}
	if (Score < 70.0f)
	{
		return TEXT("NORMAL");
	}
	if (Score < 90.0f)
	{
		return TEXT("GOOD");
	}
	return TEXT("SPECIAL");
}

FLinearColor GetCookingMinigameQualityColor(float Score)
{
	if (Score < 40.0f)
	{
		return FLinearColor(0.95f, 0.22f, 0.18f, 1.0f);
	}
	if (Score < 70.0f)
	{
		return FLinearColor(0.95f, 0.92f, 0.78f, 1.0f);
	}
	if (Score < 90.0f)
	{
		return FLinearColor(0.30f, 0.85f, 1.0f, 1.0f);
	}
	return FLinearColor(1.0f, 0.78f, 0.22f, 1.0f);
}
}

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

	if (MinigameScore <= 0.0f)
	{
		FinishCookingMinigame();
		return;
	}

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
	QualityTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigameQuality_Runtime"));
	GaugeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CookingMinigameGaugeCanvas_Runtime"));
	GaugeTrackBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CookingMinigameGaugeTrack_Runtime"));
	GaugeSuccessFillBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CookingMinigameGaugeSuccess_Runtime"));
	TargetZoneBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CookingMinigameTargetZone_Runtime"));
	PlayerNeedleBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CookingMinigamePlayerNeedle_Runtime"));
	TargetMarkerTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigameTargetMarker_Runtime"));
	PlayerMarkerTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CookingMinigamePlayerMarker_Runtime"));
	TimeProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CookingMinigameTime_Runtime"));

	if (InstructionTextBlock == nullptr || ScoreTextBlock == nullptr || TargetLabelTextBlock == nullptr ||
		PlayerLabelTextBlock == nullptr || TimeLabelTextBlock == nullptr || QualityTextBlock == nullptr ||
		GaugeCanvas == nullptr || GaugeTrackBorder == nullptr || GaugeSuccessFillBorder == nullptr ||
		TargetZoneBorder == nullptr || PlayerNeedleBorder == nullptr || TargetMarkerTextBlock == nullptr ||
		PlayerMarkerTextBlock == nullptr || TimeProgressBar == nullptr)
	{
		return;
	}

	RootCanvas->SetVisibility(ESlateVisibility::Visible);
	RootBorder->SetBrushColor(FLinearColor(0.014f, 0.018f, 0.025f, 0.88f));
	RootBorder->SetPadding(FMargin(46.0f, 34.0f));
	RootBorder->SetContent(LayoutBox);
	PanelSizeBox->SetWidthOverride(1160.0f);
	PanelSizeBox->SetHeightOverride(430.0f);
	PanelSizeBox->SetContent(RootBorder);

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSizeBox);
	if (PanelSlot != nullptr)
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
		PanelSlot->SetSize(FVector2D(1160.0f, 430.0f));
	}

	FSlateFontInfo TitleFont = InstructionTextBlock->GetFont();
	TitleFont.Size = 36;
	FSlateFontInfo ScoreFont = ScoreTextBlock->GetFont();
	ScoreFont.Size = 30;
	FSlateFontInfo LabelFont = TargetLabelTextBlock->GetFont();
	LabelFont.Size = 25;
	FSlateFontInfo MarkerFont = TargetMarkerTextBlock->GetFont();
	MarkerFont.Size = 28;

	InstructionTextBlock->SetText(FText::FromString(TEXT("조리 타이밍")));
	InstructionTextBlock->SetFont(TitleFont);
	InstructionTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.86f, 0.45f, 1.0f)));
	ScoreTextBlock->SetFont(ScoreFont);
	ScoreTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	QualityTextBlock->SetFont(ScoreFont);
	TargetLabelTextBlock->SetText(FText::FromString(TEXT("초록 영역 = 목표")));
	PlayerLabelTextBlock->SetText(FText::FromString(TEXT("노란 바늘 = 내 위치")));
	TimeLabelTextBlock->SetText(FText::FromString(TEXT("남은 시간")));
	TargetLabelTextBlock->SetFont(LabelFont);
	PlayerLabelTextBlock->SetFont(LabelFont);
	TimeLabelTextBlock->SetFont(LabelFont);
	TargetMarkerTextBlock->SetText(FText::FromString(TEXT("목표")));
	PlayerMarkerTextBlock->SetText(FText::FromString(TEXT("내 위치")));
	TargetMarkerTextBlock->SetFont(MarkerFont);
	PlayerMarkerTextBlock->SetFont(MarkerFont);
	TargetLabelTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 1.0f, 0.62f, 1.0f)));
	PlayerLabelTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.20f, 1.0f)));
	TimeLabelTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.92f, 1.0f, 1.0f)));
	TargetMarkerTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.12f, 1.0f, 0.48f, 1.0f)));
	PlayerMarkerTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.20f, 1.0f)));

	GaugeTrackBorder->SetBrushColor(FLinearColor(0.04f, 0.065f, 0.075f, 0.96f));
	GaugeSuccessFillBorder->SetBrushColor(FLinearColor(0.08f, 0.78f, 0.42f, 0.42f));
	TargetZoneBorder->SetBrushColor(FLinearColor(0.18f, 1.0f, 0.52f, 0.82f));
	PlayerNeedleBorder->SetBrushColor(FLinearColor(1.0f, 0.78f, 0.08f, 1.0f));
	TimeProgressBar->SetFillColorAndOpacity(FLinearColor(0.35f, 0.86f, 1.0f, 1.0f));

	USizeBox* GaugeSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CookingMinigameGaugeSize_Runtime"));
	USizeBox* TimeBarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CookingMinigameTimeSize_Runtime"));
	if (GaugeSizeBox == nullptr || TimeBarSizeBox == nullptr)
	{
		return;
	}

	GaugeSizeBox->SetWidthOverride(GaugeWidth);
	GaugeSizeBox->SetHeightOverride(GaugeHeight);
	GaugeSizeBox->SetContent(GaugeCanvas);
	TimeBarSizeBox->SetWidthOverride(GaugeWidth);
	TimeBarSizeBox->SetHeightOverride(30.0f);
	TimeBarSizeBox->SetContent(TimeProgressBar);

	if (UCanvasPanelSlot* TrackSlot = GaugeCanvas->AddChildToCanvas(GaugeTrackBorder))
	{
		TrackSlot->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
		TrackSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		TrackSlot->SetPosition(FVector2D(0.0f, 7.0f));
		TrackSlot->SetSize(FVector2D(GaugeWidth, 26.0f));
		TrackSlot->SetZOrder(0);
	}
	if (UCanvasPanelSlot* SuccessSlot = GaugeCanvas->AddChildToCanvas(GaugeSuccessFillBorder))
	{
		SuccessSlot->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
		SuccessSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		SuccessSlot->SetPosition(FVector2D(0.0f, 7.0f));
		SuccessSlot->SetSize(FVector2D(0.0f, 26.0f));
		SuccessSlot->SetZOrder(1);
	}
	if (UCanvasPanelSlot* TargetSlot = GaugeCanvas->AddChildToCanvas(TargetZoneBorder))
	{
		TargetSlot->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
		TargetSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		TargetSlot->SetPosition(FVector2D(GaugeWidth * 0.5f, 7.0f));
		TargetSlot->SetSize(FVector2D(GaugeWidth * TargetZoneSize, 36.0f));
		TargetSlot->SetZOrder(2);
	}
	if (UCanvasPanelSlot* PlayerSlot = GaugeCanvas->AddChildToCanvas(PlayerNeedleBorder))
	{
		PlayerSlot->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
		PlayerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PlayerSlot->SetPosition(FVector2D(GaugeWidth * 0.5f, 7.0f));
		PlayerSlot->SetSize(FVector2D(10.0f, 72.0f));
		PlayerSlot->SetZOrder(4);
	}
	if (UCanvasPanelSlot* TargetTextSlot = GaugeCanvas->AddChildToCanvas(TargetMarkerTextBlock))
	{
		TargetTextSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		TargetTextSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		TargetTextSlot->SetPosition(FVector2D(GaugeWidth * 0.5f, 0.0f));
		TargetTextSlot->SetSize(FVector2D(120.0f, 30.0f));
		TargetTextSlot->SetZOrder(3);
	}
	if (UCanvasPanelSlot* PlayerTextSlot = GaugeCanvas->AddChildToCanvas(PlayerMarkerTextBlock))
	{
		PlayerTextSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		PlayerTextSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		PlayerTextSlot->SetPosition(FVector2D(GaugeWidth * 0.5f, 0.0f));
		PlayerTextSlot->SetSize(FVector2D(140.0f, 30.0f));
		PlayerTextSlot->SetZOrder(5);
	}

	if (UVerticalBoxSlot* InstructionSlot = LayoutBox->AddChildToVerticalBox(InstructionTextBlock))
	{
		InstructionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}
	if (UVerticalBoxSlot* ScoreSlot = LayoutBox->AddChildToVerticalBox(ScoreTextBlock))
	{
		ScoreSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}
	if (UVerticalBoxSlot* QualitySlot = LayoutBox->AddChildToVerticalBox(QualityTextBlock))
	{
		QualitySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}
	LayoutBox->AddChildToVerticalBox(TargetLabelTextBlock);
	LayoutBox->AddChildToVerticalBox(PlayerLabelTextBlock);
	if (UVerticalBoxSlot* GaugeSlot = LayoutBox->AddChildToVerticalBox(GaugeSizeBox))
	{
		GaugeSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 22.0f));
	}
	LayoutBox->AddChildToVerticalBox(TimeLabelTextBlock);
	if (UVerticalBoxSlot* TimeBarSlot = LayoutBox->AddChildToVerticalBox(TimeBarSizeBox))
	{
		TimeBarSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	}

	WidgetTree->RootWidget = RootCanvas;
	RefreshDefaultMinigameVisuals();
}

void UCookingMinigameWidget::RefreshDefaultMinigameVisuals()
{
	if (ScoreTextBlock != nullptr)
	{
		ScoreTextBlock->SetText(FText::FromString(FString::Printf(
			TEXT("점수 %d"),
			FMath::RoundToInt(MinigameScore)
		)));
	}

	if (QualityTextBlock != nullptr)
	{
		QualityTextBlock->SetText(FText::FromString(GetCookingMinigameQualityText(MinigameScore)));
		QualityTextBlock->SetColorAndOpacity(FSlateColor(GetCookingMinigameQualityColor(MinigameScore)));
	}

	const float ClampedTargetPosition = FMath::Clamp(TargetPosition, 0.0f, 1.0f);
	const float ClampedPlayerPosition = FMath::Clamp(PlayerBarPosition, 0.0f, 1.0f);
	const float TargetZoneWidth = FMath::Clamp(TargetZoneSize, 0.02f, 1.0f) * GaugeWidth;

	if (GaugeSuccessFillBorder != nullptr)
	{
		if (UCanvasPanelSlot* SuccessSlot = Cast<UCanvasPanelSlot>(GaugeSuccessFillBorder->Slot))
		{
			SuccessSlot->SetSize(FVector2D(ClampedPlayerPosition * GaugeWidth, 26.0f));
		}
		GaugeSuccessFillBorder->SetBrushColor(IsPlayerBarInTargetZone()
			? FLinearColor(0.18f, 0.95f, 0.45f, 0.56f)
			: FLinearColor(0.10f, 0.52f, 0.72f, 0.36f));
	}

	if (TargetZoneBorder != nullptr)
	{
		if (UCanvasPanelSlot* TargetSlot = Cast<UCanvasPanelSlot>(TargetZoneBorder->Slot))
		{
			TargetSlot->SetPosition(FVector2D(ClampedTargetPosition * GaugeWidth, 7.0f));
			TargetSlot->SetSize(FVector2D(TargetZoneWidth, 36.0f));
		}
	}

	if (PlayerNeedleBorder != nullptr)
	{
		if (UCanvasPanelSlot* PlayerSlot = Cast<UCanvasPanelSlot>(PlayerNeedleBorder->Slot))
		{
			PlayerSlot->SetPosition(FVector2D(ClampedPlayerPosition * GaugeWidth, 7.0f));
		}
		PlayerNeedleBorder->SetBrushColor(IsPlayerBarInTargetZone()
			? FLinearColor(1.0f, 0.90f, 0.18f, 1.0f)
			: FLinearColor(1.0f, 0.42f, 0.12f, 1.0f));
	}

	if (TargetMarkerTextBlock != nullptr)
	{
		if (UCanvasPanelSlot* TargetTextSlot = Cast<UCanvasPanelSlot>(TargetMarkerTextBlock->Slot))
		{
			TargetTextSlot->SetPosition(FVector2D(ClampedTargetPosition * GaugeWidth, 0.0f));
		}
	}

	if (PlayerMarkerTextBlock != nullptr)
	{
		if (UCanvasPanelSlot* PlayerTextSlot = Cast<UCanvasPanelSlot>(PlayerMarkerTextBlock->Slot))
		{
			PlayerTextSlot->SetPosition(FVector2D(ClampedPlayerPosition * GaugeWidth, 0.0f));
		}
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
