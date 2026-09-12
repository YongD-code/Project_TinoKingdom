// Fill out your copyright notice in the Description page of Project Settings.


#include "EndScreenWidget.h"

void UEndScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	HideEndScreen();
}

void UEndScreenWidget::ShowEndScreen()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEndScreenWidget::HideEndScreen()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

