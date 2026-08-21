// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TinoPlayerWidget.generated.h"

class UImage;
class UCanvasPanelSlot;
class AActor;

UCLASS()
class PROJECT_TINOKINGDOM_API UTinoPlayerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetCrosshairVisible(bool bVisible);
	void SetLockOnMarkerTarget(AActor* NewTarget);
	
protected:
	virtual void NativeOnInitialized() override;
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Crosshair;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LockOnMarker;
	
private:
	void UpdateLockOnMarkerPosition();
	
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanelSlot> LockOnMarkerSlot;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LockOnTarget;
};
