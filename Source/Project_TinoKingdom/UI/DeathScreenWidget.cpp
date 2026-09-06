// Fill out your copyright notice in the Description page of Project Settings.

#include "DeathScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"
#include "Sound/SoundBase.h"

void UDeathScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildDefaultVisuals();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDeathScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bFadingTextIn && !bFadingToBlack)
	{
		return;
	}

	const double ElapsedRealTime = FPlatformTime::Seconds() - FadeStartRealTime;
	if (bFadingTextIn)
	{
		const float Alpha = TextFadeInDuration <= 0.0f
			? 1.0f
			: FMath::Clamp(static_cast<float>(ElapsedRealTime) / TextFadeInDuration, 0.0f, 1.0f);
		if (DeathMessageText != nullptr)
		{
			DeathMessageText->SetRenderOpacity(FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f));
		}
		if (Alpha >= 1.0f)
		{
			bFadingTextIn = false;
		}
	}

	if (bFadingToBlack)
	{
		const float Alpha = FadeToBlackDuration <= 0.0f
			? 1.0f
			: FMath::Clamp(static_cast<float>(ElapsedRealTime) / FadeToBlackDuration, 0.0f, 1.0f);
		if (DeathFadeBorder != nullptr)
		{
			DeathFadeBorder->SetRenderOpacity(Alpha);
		}
		if (DeathMessageText != nullptr)
		{
			DeathMessageText->SetRenderOpacity(1.0f - Alpha);
		}
		if (Alpha >= 1.0f)
		{
			bFadingToBlack = false;
		}
	}
}

void UDeathScreenWidget::ShowDeathMessage(AActor* DamageCauser)
{
	BuildDefaultVisuals();
	if (IsValid(DeathFadeSound))
	{
		PlaySound(DeathFadeSound);
	}
	
	if (DeathMessageText != nullptr)
	{
		DeathMessageText->SetText(MakeDeathMessage(DamageCauser));
	}

	FadeStartRealTime = FPlatformTime::Seconds();
	bFadingTextIn = true;
	bFadingToBlack = false;
	if (DeathMessageText != nullptr)
	{
		DeathMessageText->SetRenderOpacity(TextFadeInDuration <= 0.0f ? 1.0f : 0.0f);
	}
	if (DeathFadeBorder != nullptr)
	{
		DeathFadeBorder->SetRenderOpacity(0.0f);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDeathScreenWidget::FadeToBlack(float Duration)
{
	bFadingTextIn = false;
	FadeToBlackDuration = FMath::Max(Duration, 0.0f);
	if (FadeToBlackDuration <= 0.0f)
	{
		bFadingToBlack = false;
		if (DeathFadeBorder != nullptr)
		{
			DeathFadeBorder->SetRenderOpacity(1.0f);
		}
		if (DeathMessageText != nullptr)
		{
			DeathMessageText->SetRenderOpacity(0.0f);
		}
		return;
	}

	bFadingToBlack = true;
	FadeStartRealTime = FPlatformTime::Seconds();
}

void UDeathScreenWidget::HideDeathMessage()
{
	bFadingTextIn = false;
	bFadingToBlack = false;
	if (DeathMessageText != nullptr)
	{
		DeathMessageText->SetRenderOpacity(1.0f);
	}
	if (DeathFadeBorder != nullptr)
	{
		DeathFadeBorder->SetRenderOpacity(0.0f);
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDeathScreenWidget::BuildDefaultVisuals()
{
	if ((DeathMessageText != nullptr && DeathFadeBorder != nullptr) || WidgetTree == nullptr)
	{
		return;
	}

	if (GetRootWidget() != nullptr)
	{
		DeathMessageText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DeathMessageText")));
		DeathFadeBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("DeathFadeBorder")));
		if (DeathFadeBorder == nullptr)
		{
			if (UCanvasPanel* ExistingRootCanvas = Cast<UCanvasPanel>(GetRootWidget()))
			{
				DeathFadeBorder = WidgetTree->ConstructWidget<UBorder>(
					UBorder::StaticClass(), TEXT("DeathFadeBorder_Runtime"));
				DeathFadeBorder->SetBrushColor(FLinearColor::Black);
				DeathFadeBorder->SetRenderOpacity(0.0f);
				if (UCanvasPanelSlot* FadeSlot = ExistingRootCanvas->AddChildToCanvas(DeathFadeBorder))
				{
					FadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
					FadeSlot->SetOffsets(FMargin(0.0f));
					FadeSlot->SetZOrder(0);
				}
			}
		}
		if (DeathMessageText != nullptr)
		{
			if (UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(DeathMessageText->Slot))
			{
				TextSlot->SetZOrder(1);
			}
		}
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("DeathScreenRoot_Runtime"));
	DeathFadeBorder = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("DeathFadeBorder"));
	DeathMessageText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("DeathMessageText"));
	if (RootCanvas == nullptr || DeathFadeBorder == nullptr || DeathMessageText == nullptr)
	{
		return;
	}

	DeathFadeBorder->SetBrushColor(FLinearColor::Black);
	DeathFadeBorder->SetRenderOpacity(0.0f);
	if (UCanvasPanelSlot* FadeSlot = RootCanvas->AddChildToCanvas(DeathFadeBorder))
	{
		FadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FadeSlot->SetOffsets(FMargin(0.0f));
		FadeSlot->SetZOrder(0);
	}

	FSlateFontInfo Font = DeathMessageText->GetFont();
	Font.Size = 44;
	DeathMessageText->SetFont(Font);
	DeathMessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DeathMessageText->SetJustification(ETextJustify::Center);
	DeathMessageText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	DeathMessageText->SetShadowOffset(FVector2D(2.0f, 2.0f));

	if (UCanvasPanelSlot* TextSlot = RootCanvas->AddChildToCanvas(DeathMessageText))
	{
		TextSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		TextSlot->SetPosition(FVector2D::ZeroVector);
		TextSlot->SetSize(FVector2D(1200.0f, 120.0f));
		TextSlot->SetZOrder(1);
	}

	WidgetTree->RootWidget = RootCanvas;
}

FText UDeathScreenWidget::MakeDeathMessage(AActor* DamageCauser) const
{
	const AActor* DisplayActor = DamageCauser;
	if (IsValid(DamageCauser))
	{
		if (const APawn* InstigatorPawn = DamageCauser->GetInstigator())
		{
			DisplayActor = InstigatorPawn;
		}
	}

	const FText CauserName = IsValid(DisplayActor)
		? FText::FromString(DisplayActor->GetActorNameOrLabel())
		: NSLOCTEXT("DeathScreen", "UnknownDamageCauser", "알 수 없는 원인");

	return FText::Format(
		NSLOCTEXT("DeathScreen", "DeathMessage", "{0}에 의해 처참히 사망했습니다."),
		CauserName);
}
