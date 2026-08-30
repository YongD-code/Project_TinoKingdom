// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "CookingWidget.generated.h"

class UCookingComponent;
class UButton;
class UEditableTextBox;
class UTextBlock;

UCLASS()
class PROJECT_TINOKINGDOM_API UCookingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void InitializeCookingWidget(UCookingComponent* InCookingComponent, UInventoryComponent* InInventoryComponent);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	bool AddIngredientFromInventory(const FInventoryItemStack& Ingredient);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	bool AddFirstAvailableIngredientFromInventory();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void RemoveIngredientAt(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void ClearIngredients();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	bool CompleteCooking(float MinigameScore, FCookingResultData& OutResult);

	UFUNCTION(BlueprintPure, Category = "Cooking")
	UCookingComponent* GetCookingComponent() const { return CookingComponent; }

	UFUNCTION(BlueprintPure, Category = "Cooking")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Cooking")
	void OnSelectedIngredientsChanged(const TArray<FInventoryItemStack>& SelectedIngredients);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cooking")
	void OnCookingCompleted(const FCookingResultData& ResultData);

private:
	void BroadcastSelectedIngredientsChanged();
	void UpdateIngredientSlotTexts(const TArray<FInventoryItemStack>& SelectedIngredients);
	void SetIngredientSlotText(int32 Index, const FText& Text);
	void SetResultText(const FText& Text);

	UFUNCTION()
	void HandleStartCookingClicked();

	UPROPERTY(Transient)
	TObjectPtr<UCookingComponent> CookingComponent;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_StartCooking;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> TextBox_Result;

	UPROPERTY(EditDefaultsOnly, Category = "Cooking", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float DefaultCookingScore = 75.0f;
};
