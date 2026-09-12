// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndScreenWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API UEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowEndScreen();
	void HideEndScreen();

protected:
	virtual void NativeOnInitialized() override;
};
