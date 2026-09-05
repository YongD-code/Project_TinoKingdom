#include "EnemyHealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"

void UEnemyHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureHealthBarWidget();
}

void UEnemyHealthBarWidget::SetHealthPercent(float HealthPercent)
{
	EnsureHealthBarWidget();
	if (HealthProgressBar == nullptr)
	{
		return;
	}

	const float ClampedPercent = FMath::Clamp(HealthPercent, 0.0f, 1.0f);
	HealthProgressBar->SetPercent(ClampedPercent);
	HealthProgressBar->SetFillColorAndOpacity(GetHealthColor(ClampedPercent));
}

void UEnemyHealthBarWidget::EnsureHealthBarWidget()
{
	if (HealthProgressBar != nullptr || WidgetTree == nullptr)
	{
		return;
	}

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EnemyHealthBarRoot"));
	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("EnemyHealthProgressBar"));

	if (RootBorder == nullptr || HealthProgressBar == nullptr)
	{
		return;
	}

	RootBorder->SetBrushColor(FLinearColor(0.015f, 0.012f, 0.01f, 0.82f));
	RootBorder->SetPadding(FMargin(2.0f));
	RootBorder->SetContent(HealthProgressBar);

	HealthProgressBar->SetPercent(1.0f);
	HealthProgressBar->SetFillColorAndOpacity(GetHealthColor(1.0f));

	WidgetTree->RootWidget = RootBorder;
}

FLinearColor UEnemyHealthBarWidget::GetHealthColor(float HealthPercent) const
{
	if (HealthPercent <= 0.3f)
	{
		return FLinearColor(0.95f, 0.12f, 0.08f, 1.0f);
	}
	if (HealthPercent <= 0.7f)
	{
		return FLinearColor(1.0f, 0.78f, 0.08f, 1.0f);
	}

	return FLinearColor(0.42f, 0.95f, 0.22f, 1.0f);
}
