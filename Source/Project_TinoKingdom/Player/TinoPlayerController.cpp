// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/UserWidget.h"
#include "Project_TinoKingdom/UI/TinoPlayerWidget.h"


void ATinoPlayerController::SetCrosshairVisible(bool bVisible)
{
	if (PlayerUIWidget != nullptr)
	{
		PlayerUIWidget->SetCrosshairVisible(bVisible);
	}
}

void ATinoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (PlayerUIClass != nullptr)
	{
		PlayerUIWidget = CreateWidget<UTinoPlayerWidget>(this,PlayerUIClass);

		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->AddToViewport();
		}
	}
	
	if (DefaultMappingContext == nullptr)
	{
		return;
	}
	
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (LocalPlayer == nullptr)
	{
		return;
	}
	
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = 
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	
	if (InputSubsystem != nullptr)
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext,0);
	}
}
