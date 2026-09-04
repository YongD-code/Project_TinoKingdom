// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputKeyEventArgs.h"
#include "Blueprint/UserWidget.h"
#include "Project_TinoKingdom/Component/CookingComponent.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/UI/CookingWidget.h"
#include "Project_TinoKingdom/UI/TinoPlayerWidget.h"


void ATinoPlayerController::SetPlayerUIVisible(bool bVisible)
{
	if (PlayerUIWidget != nullptr)
	{
		PlayerUIWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void ATinoPlayerController::SetCrosshairVisible(bool bVisible)
{
	if (PlayerUIWidget != nullptr)
	{
		PlayerUIWidget->SetCrosshairVisible(bVisible);
	}
}

void ATinoPlayerController::SetLockOnMarkerTarget(AActor* NewTarget)
{
	if (PlayerUIWidget != nullptr)
	{
		PlayerUIWidget->SetLockOnMarkerTarget(NewTarget);
	}
}

void ATinoPlayerController::ToggleCharacterMenu()
{
	if (bCookingMenuOpen)
	{
		ToggleCookingMenu(nullptr, nullptr);
	}

	if (PlayerUIWidget == nullptr)
	{
		return;
	}

	bCharacterMenuOpen = !bCharacterMenuOpen;
	PlayerUIWidget->SetCharacterMenuVisible(bCharacterMenuOpen);
	
	if (bCharacterMenuOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		SetPause(true);
		
		return;
	}
	
	SetPause(false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void ATinoPlayerController::ToggleCookingMenu(UCookingComponent* CookingComponent, UInventoryComponent* InventoryComponent)
{
	if (bCharacterMenuOpen)
	{
		ToggleCharacterMenu();
	}

	bCookingMenuOpen = !bCookingMenuOpen;

	if (!bCookingMenuOpen)
	{
		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->CloseCookingIngredientPicker();
		}

		if (CookingUIWidget != nullptr)
		{
			CookingUIWidget->RemoveFromParent();
		}

		SetPause(false);
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		return;
	}

	if (CookingUIClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("CookingUIClass가 지정되지 않았습니다."));
		bCookingMenuOpen = false;
		return;
	}

	if (CookingComponent == nullptr || InventoryComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cooking UI를 열 수 없습니다. CookingComponent 또는 InventoryComponent가 없습니다."));
		bCookingMenuOpen = false;
		return;
	}

	if (CookingUIWidget == nullptr)
	{
		CookingUIWidget = CreateWidget<UCookingWidget>(this, CookingUIClass);
	}

	if (CookingUIWidget == nullptr)
	{
		bCookingMenuOpen = false;
		return;
	}

	CookingUIWidget->InitializeCookingWidget(CookingComponent, InventoryComponent);
	CookingUIWidget->AddToViewport(5);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(CookingUIWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
	SetPause(true);
}

bool ATinoPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (Params.Event == IE_Pressed && Params.Key == EKeys::C)
	{
		if (bCookingMenuOpen)
		{
			ToggleCookingMenu(nullptr, nullptr);
			return true;
		}

		if (APlayerCharacter* TinoPlayerCharacter = Cast<APlayerCharacter>(GetPawn()))
		{
			ToggleCookingMenu(
				TinoPlayerCharacter->GetCookingComponent(),
				TinoPlayerCharacter->GetInventoryComponent()
			);
			return true;
		}
	}

	return Super::InputKey(Params);
}

void ATinoPlayerController::ShowCookingIngredientPicker(UCookingWidget* CookingWidget, UInventoryComponent* InventoryComponent)
{
	if (PlayerUIWidget == nullptr)
	{
		return;
	}

	PlayerUIWidget->ShowCookingIngredientPicker(CookingWidget, InventoryComponent);
	PlayerUIWidget->RemoveFromParent();
	PlayerUIWidget->AddToViewport(4);
}

void ATinoPlayerController::RefreshCookingIngredientPicker()
{
	if (PlayerUIWidget != nullptr)
	{
		PlayerUIWidget->RefreshCookingIngredientPicker();
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
