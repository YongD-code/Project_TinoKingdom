// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TinoPlayerWidget.generated.h"

class UImage;
/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API UTinoPlayerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetCrosshairVisible(bool bVisible);
	
protected:
	virtual void NativeOnInitialized() override;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Crosshair;
};
