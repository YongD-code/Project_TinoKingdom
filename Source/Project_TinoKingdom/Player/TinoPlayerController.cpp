// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/UserWidget.h"


void ATinoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
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
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	
	if (PlayerUIClass != nullptr)
	{
		PlayerUIWidget = CreateWidget<UUserWidget>(this, PlayerUIClass);
		
		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->AddToViewport();
		}
	}
}
