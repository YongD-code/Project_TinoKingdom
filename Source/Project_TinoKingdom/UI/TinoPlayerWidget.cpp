// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoPlayerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Project_TinoKingdom/Interface/TargetableInterface.h"

void UTinoPlayerWidget::SetCrosshairVisible(bool bVisible)
{
	Crosshair->SetVisibility((bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed));
}

void UTinoPlayerWidget::SetLockOnMarkerTarget(AActor* NewTarget)
{
	LockOnTarget = NewTarget;
	if (!LockOnTarget.IsValid())
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	UpdateLockOnMarkerPosition();
}

void UTinoPlayerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	LockOnMarkerSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(LockOnMarker);
	
	SetCrosshairVisible(false);
	SetLockOnMarkerTarget(nullptr);
}

void UTinoPlayerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (!LockOnTarget.IsValid())
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	UpdateLockOnMarkerPosition();
}

void UTinoPlayerWidget::UpdateLockOnMarkerPosition()
{
	AActor* TargetActor = LockOnTarget.Get();
	const FVector LockOnLocation = ITargetableInterface::Execute_GetLockOnLocation(TargetActor);
	
	FVector2D MarkerPosition;
	const bool bProjected = UWidgetLayoutLibrary::
		ProjectWorldLocationToWidgetPosition(
			GetOwningPlayer(),
			LockOnLocation,
			MarkerPosition,
			true
		);
	
	if (!bProjected)
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	LockOnMarkerSlot->SetPosition(MarkerPosition);
	LockOnMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
}
