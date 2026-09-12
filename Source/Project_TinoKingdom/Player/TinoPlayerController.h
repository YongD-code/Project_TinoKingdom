// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TinoPlayerController.generated.h"

class UInputMappingContext;
class UTinoPlayerWidget;
class UCookingComponent;
class UCookingWidget;
class UInventoryComponent;
class UInputAction;
class UUserWidget;
class UDeathScreenWidget;
class UEndScreenWidget;

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ATinoPlayerController();

	void SetPlayerUIVisible(bool bVisible);
	void SetCrosshairVisible(bool bVisible);
	void SetLockOnMarkerTarget(AActor* NewTarget);
	void SetMenuBackgroundVisible(bool bVisible);
	void CloseAllMenus();
	void ShowDeathScreen(AActor* DamageCauser);
	void FadeDeathScreenToBlack(float Duration);
	void HideDeathScreen();

	// 엔딩 조건이 충족됐을 때 호출한다. 실제 레벨 이동은 EndScreenContinueAction 입력에서 처리한다.
	UFUNCTION(BlueprintCallable, Category = "UI|End Screen")
	void ShowEndScreen();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void ResetGameInputMode();
	
	void ToggleCharacterMenu();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void ToggleCookingMenu(UCookingComponent* CookingComponent, UInventoryComponent* InventoryComponent);

	void ShowCookingIngredientPicker(UCookingWidget* CookingWidget, UInventoryComponent* InventoryComponent);
	void RefreshCookingIngredientPicker();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// 에디터에서 Enter 키가 매핑된 Input Action을 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|End Screen")
	TObjectPtr<UInputAction> EndScreenContinueAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UTinoPlayerWidget> PlayerUIClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Cooking")
	TSubclassOf<UCookingWidget> CookingUIClass;
	
	UPROPERTY()
	TObjectPtr<UTinoPlayerWidget> PlayerUIWidget;

	UPROPERTY()
	TObjectPtr<UCookingWidget> CookingUIWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Menu")
	TSoftClassPtr<UUserWidget> MenuBackgroundClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MenuBackgroundWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Death")
	TSoftClassPtr<UDeathScreenWidget> DeathScreenClass;

	UPROPERTY()
	TObjectPtr<UDeathScreenWidget> DeathScreenWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|End Screen")
	TSoftClassPtr<UEndScreenWidget> EndScreenClass;

	UPROPERTY()
	TObjectPtr<UEndScreenWidget> EndScreenWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Travel|End Screen")
	FName EndScreenDestinationLevel = TEXT("/Game/Map/TinoKingdom_ByChanWoong");
	
private:
	void EnsureMenuBackgroundWidget();
	void EnsureDeathScreenWidget();
	void EnsureEndScreenWidget();
	void HandleEndScreenContinue();
	void ResetGameInputModeForCurrentMap();

	bool bCharacterMenuOpen = false;
	bool bCookingMenuOpen = false;
	bool bEndScreenOpen = false;
	bool bEndScreenTravelInProgress = false;
};
