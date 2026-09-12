// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/Component/CookingComponent.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/GameMode/TinoGameInstance.h"
#include "Project_TinoKingdom/UI/CookingWidget.h"
#include "Project_TinoKingdom/UI/DeathScreenWidget.h"
#include "Project_TinoKingdom/UI/EndScreenWidget.h"
#include "Project_TinoKingdom/UI/TinoPlayerWidget.h"

ATinoPlayerController::ATinoPlayerController()
{
	MenuBackgroundClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(
		TEXT("/Game/UI/WBP_MenuBackground.WBP_MenuBackground_C")));
	DeathScreenClass = TSoftClassPtr<UDeathScreenWidget>(FSoftObjectPath(
		TEXT("/Game/UI/WBP_DeathScreen.WBP_DeathScreen_C")));
	EndScreenClass = TSoftClassPtr<UEndScreenWidget>(FSoftObjectPath(
		TEXT("/Game/UI/WBP_EndScreen.WBP_EndScreen_C")));
}

void ATinoPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent != nullptr && EndScreenContinueAction != nullptr)
	{
		EnhancedInputComponent->BindAction(
			EndScreenContinueAction,
			ETriggerEvent::Started,
			this,
			&ATinoPlayerController::HandleEndScreenContinue);
	}
}

void ATinoPlayerController::EnsureMenuBackgroundWidget()
{
	if (MenuBackgroundWidget != nullptr)
	{
		return;
	}

	UClass* BackgroundClass = MenuBackgroundClass.LoadSynchronous();
	if (!IsValid(BackgroundClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("MenuBackgroundClass를 불러오지 못했습니다."));
		return;
	}

	MenuBackgroundWidget = CreateWidget<UUserWidget>(this, BackgroundClass);
	if (MenuBackgroundWidget != nullptr)
	{
		MenuBackgroundWidget->AddToViewport(-100);
		MenuBackgroundWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ATinoPlayerController::SetMenuBackgroundVisible(bool bVisible)
{
	EnsureMenuBackgroundWidget();
	if (MenuBackgroundWidget != nullptr)
	{
		MenuBackgroundWidget->SetVisibility(
			bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ATinoPlayerController::EnsureDeathScreenWidget()
{
	if (DeathScreenWidget != nullptr)
	{
		return;
	}

	UClass* WidgetClass = DeathScreenClass.LoadSynchronous();
	if (!IsValid(WidgetClass))
	{
		WidgetClass = UDeathScreenWidget::StaticClass();
	}

	DeathScreenWidget = CreateWidget<UDeathScreenWidget>(this, WidgetClass);
	if (DeathScreenWidget != nullptr)
	{
		DeathScreenWidget->AddToViewport(100);
		DeathScreenWidget->HideDeathMessage();
	}
}

void ATinoPlayerController::ShowDeathScreen(AActor* DamageCauser)
{
	EnsureDeathScreenWidget();
	if (DeathScreenWidget != nullptr)
	{
		DeathScreenWidget->ShowDeathMessage(DamageCauser);
	}
}

void ATinoPlayerController::FadeDeathScreenToBlack(float Duration)
{
	if (DeathScreenWidget != nullptr)
	{
		DeathScreenWidget->FadeToBlack(Duration);
	}
}

void ATinoPlayerController::HideDeathScreen()
{
	if (DeathScreenWidget != nullptr)
	{
		DeathScreenWidget->HideDeathMessage();
	}
}

void ATinoPlayerController::EnsureEndScreenWidget()
{
	if (EndScreenWidget != nullptr)
	{
		return;
	}

	UClass* WidgetClass = EndScreenClass.LoadSynchronous();
	if (!IsValid(WidgetClass))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("EndScreenClass를 불러오지 못해 C++ 기본 EndScreenWidget을 사용합니다."));
		WidgetClass = UEndScreenWidget::StaticClass();
	}

	EndScreenWidget = CreateWidget<UEndScreenWidget>(this, WidgetClass);
	if (EndScreenWidget != nullptr)
	{
		EndScreenWidget->AddToViewport(200);
		EndScreenWidget->HideEndScreen();
	}
}

void ATinoPlayerController::ShowEndScreen()
{
	if (bEndScreenOpen || bEndScreenTravelInProgress)
	{
		return;
	}

	EnsureEndScreenWidget();
	if (EndScreenWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("End Screen 위젯을 생성하지 못했습니다."));
		return;
	}

	CloseAllMenus();
	bEndScreenOpen = true;
	SetPlayerUIVisible(false);
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	EndScreenWidget->ShowEndScreen();
}

void ATinoPlayerController::HandleEndScreenContinue()
{
	if (!bEndScreenOpen || bEndScreenTravelInProgress || EndScreenDestinationLevel.IsNone())
	{
		return;
	}

	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn());
	UTinoGameInstance* TinoGameInstance = Cast<UTinoGameInstance>(GetGameInstance());
	if (!IsValid(PlayerCharacter) || !ensureMsgf(TinoGameInstance != nullptr,
		TEXT("Project Settings의 GameInstance Class가 TinoGameInstance로 지정되지 않았습니다.")))
	{
		return;
	}

	// SecretPlace로 들어갈 때와 동일한 전달 상태를 만들어 다음 맵의 PlayerCharacter가 복원하게 한다.
	if (!TinoGameInstance->CapturePlayerState(PlayerCharacter))
	{
		UE_LOG(LogTemp, Error, TEXT("End Screen 레벨 이동을 위한 플레이어 상태 저장에 실패했습니다."));
		return;
	}

	bEndScreenTravelInProgress = true;
	UGameplayStatics::OpenLevel(this, EndScreenDestinationLevel);
}

void ATinoPlayerController::ResetGameInputMode()
{
	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ATinoPlayerController::ResetGameInputModeForCurrentMap()
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FString MapName = World->GetMapName();
	if (!MapName.Contains(TEXT("SecretPlace")))
	{
		return;
	}

	ResetGameInputMode();
}

void ATinoPlayerController::CloseAllMenus()
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn());

	if (bCharacterMenuOpen)
	{
		bCharacterMenuOpen = false;
		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->SetCharacterMenuVisible(false);
		}
		if (PlayerCharacter != nullptr)
		{
			PlayerCharacter->StopSlowMotion();
		}
	}

	if (bCookingMenuOpen)
	{
		bCookingMenuOpen = false;
		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->SetCookingMenuOpen(false);
			PlayerUIWidget->CloseCookingIngredientPicker();
		}

		if (CookingUIWidget != nullptr)
		{
			CookingUIWidget->ClearIngredients();
			CookingUIWidget->RemoveFromParent();
		}

		if (PlayerCharacter != nullptr)
		{
			PlayerCharacter->StopSlowMotion();
		}
	}

	bShowMouseCursor = false;
	ResetGameInputMode();
}


void ATinoPlayerController::SetPlayerUIVisible(bool bVisible)
{
	if (PlayerUIWidget != nullptr)
	{
		PlayerUIWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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
		PlayerUIWidget->SetIsFocusable(true);

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(PlayerUIWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn()))
		{
			PlayerCharacter->StartSlowMotion();
		}
		
		return;
	}
	
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->StopSlowMotion();
	}
	bShowMouseCursor = false;
	ResetGameInputMode();
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
			PlayerUIWidget->SetCookingMenuOpen(false);
			PlayerUIWidget->CloseCookingIngredientPicker();
		}

		if (CookingUIWidget != nullptr)
		{
			CookingUIWidget->ClearIngredients();
			CookingUIWidget->RemoveFromParent();
		}

		if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn()))
		{
			PlayerCharacter->StopSlowMotion();
		}
		bShowMouseCursor = false;
		ResetGameInputMode();
		return;
	}

	if (CookingUIClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("CookingUIClass가 지정되지 않았습니다."));
		bCookingMenuOpen = false;
		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->SetCookingMenuOpen(false);
		}
		return;
	}

	if (CookingComponent == nullptr || InventoryComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cooking UI를 열 수 없습니다. CookingComponent 또는 InventoryComponent가 없습니다."));
		bCookingMenuOpen = false;
		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->SetCookingMenuOpen(false);
		}
		return;
	}

	if (CookingUIWidget == nullptr)
	{
		CookingUIWidget = CreateWidget<UCookingWidget>(this, CookingUIClass);
	}

	if (CookingUIWidget == nullptr)
	{
		bCookingMenuOpen = false;
		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->SetCookingMenuOpen(false);
		}
		return;
	}

	if (PlayerUIWidget != nullptr)
	{
		PlayerUIWidget->SetCookingMenuOpen(true);
	}

	CookingUIWidget->InitializeCookingWidget(CookingComponent, InventoryComponent);
	CookingUIWidget->AddToViewport(5);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(CookingUIWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	SetInputMode(InputMode);
	bShowMouseCursor = true;
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->StartSlowMotion();
	}
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
			if (!TinoPlayerCharacter->IsNearCookingPot())
			{
				if (PlayerUIWidget != nullptr)
				{
					PlayerUIWidget->ShowCookingUnavailableMessage();
				}
				return true;
			}

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

	PlayerUIWidget->RemoveFromParent();
	PlayerUIWidget->AddToViewport(30);
	PlayerUIWidget->ShowCookingIngredientPicker(CookingWidget, InventoryComponent);
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
	EnsureMenuBackgroundWidget();
	EnsureDeathScreenWidget();
	
	if (PlayerUIClass != nullptr)
	{
		PlayerUIWidget = CreateWidget<UTinoPlayerWidget>(this,PlayerUIClass);

		if (PlayerUIWidget != nullptr)
		{
			PlayerUIWidget->AddToViewport(4);
		}
	}

	FTimerHandle ResetInputModeTimerHandle;
	GetWorldTimerManager().SetTimer(
		ResetInputModeTimerHandle,
		this,
		&ATinoPlayerController::ResetGameInputModeForCurrentMap,
		0.1f,
		false);
	
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
