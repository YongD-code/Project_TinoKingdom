// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoPlayerWidget.h"

#include "Components/Image.h"

void UTinoPlayerWidget::SetCrosshairVisible(bool bVisible)
{
	Crosshair->SetVisibility((bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed));
}

void UTinoPlayerWidget::SetLockOnMarkerTarget(AActor* NewTarget)
{
	LockOnTarget = NewTarget;
	LockOnMarker->SetVisibility(LockOnTarget.IsValid() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UTinoPlayerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	SetCrosshairVisible(false);
	SetLockOnMarkerTarget(nullptr);
}
