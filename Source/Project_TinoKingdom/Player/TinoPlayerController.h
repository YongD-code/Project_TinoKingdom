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
class UUserWidget;
class UDeathScreenWidget;

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
	
	void ToggleCharacterMenu();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void ToggleCookingMenu(UCookingComponent* CookingComponent, UInventoryComponent* InventoryComponent);

	void ShowCookingIngredientPicker(UCookingWidget* CookingWidget, UInventoryComponent* InventoryComponent);
	void RefreshCookingIngredientPicker();
	
protected:
	virtual void BeginPlay() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
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
	
private:
	void EnsureMenuBackgroundWidget();
	void EnsureDeathScreenWidget();

	bool bCharacterMenuOpen = false;
	bool bCookingMenuOpen = false;
};
