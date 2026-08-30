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

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	void SetCrosshairVisible(bool bVisible);
	void SetLockOnMarkerTarget(AActor* NewTarget);
	
	void ToggleCharacterMenu();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void ToggleCookingMenu(UCookingComponent* CookingComponent, UInventoryComponent* InventoryComponent);

	void ShowCookingIngredientPicker(UCookingWidget* CookingWidget, UInventoryComponent* InventoryComponent);
	
protected:
	virtual void BeginPlay() override;
	
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
	
private:
	bool bCharacterMenuOpen = false;
	bool bCookingMenuOpen = false;
};
