// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Blueprint/UserWidget.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "CookingWidget.generated.h"

class UCookingComponent;
class UButton;
class UCookingMinigameWidget;
class UContentWidget;
class UEditableTextBox;
class UImage;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;

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

	const TArray<FInventoryItemStack>& GetSelectedIngredients() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Cooking")
	void OnSelectedIngredientsChanged(const TArray<FInventoryItemStack>& SelectedIngredients);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cooking")
	void OnCookingCompleted(const FCookingResultData& ResultData);

private:
	void NormalizeCookingWidgetLayering();
	void BroadcastSelectedIngredientsChanged();
	void UpdateIngredientSlotTexts(const TArray<FInventoryItemStack>& SelectedIngredients);
	void UpdateIngredientSlotImages(const TArray<FInventoryItemStack>& SelectedIngredients);
	void SetIngredientSlotText(int32 Index, const FText& Text);
	void SetIngredientSlotImage(int32 Index, UTexture2D* Icon);
	void SetIngredientSlotButtonIcon(int32 Index, UTexture2D* Icon);
	void SetIngredientSlotButtonContent(int32 Index, UTexture2D* Icon);
	void SetResultText(const FText& Text);
	void ToggleIngredientList();
	void SetIngredientListVisible(bool bVisible);
	void RefreshIngredientList();
	void SelectIngredientOption(int32 OptionIndex);
	void CloseCookingWidget();
	void OpenCookingMinigame();
	bool CanStartCooking() const;
	void RefreshCookingActions();
	void RefreshLinkedInventoryPicker();

	UFUNCTION()
	void HandleStartCookingClicked();

	UFUNCTION()
	void HandleCookingMinigameFinished(float FinalScore);

	UFUNCTION()
	void HandleCloseCookingClicked();

	UFUNCTION()
	void HandleIngredientOption0Clicked();

	UFUNCTION()
	void HandleIngredientOption1Clicked();

	UFUNCTION()
	void HandleIngredientOption2Clicked();

	UFUNCTION()
	void HandleIngredientOption3Clicked();

	UFUNCTION()
	void HandleIngredientOption4Clicked();

	UFUNCTION()
	void HandleIngredientOption5Clicked();

	UFUNCTION()
	void HandleIngredientOption6Clicked();

	UFUNCTION()
	void HandleIngredientOption7Clicked();

	UPROPERTY(Transient)
	TObjectPtr<UCookingComponent> CookingComponent;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_StartCooking;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> TextBox_Result;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> IngredientListBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseCookingButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> IngredientOptionButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> IngredientOptionTexts;

	UPROPERTY(Transient)
	TArray<FInventoryItemStack> IngredientOptions;

	UPROPERTY(EditDefaultsOnly, Category = "Cooking", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float DefaultCookingScore = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cooking|Minigame")
	TSubclassOf<UCookingMinigameWidget> CookingMinigameWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UCookingMinigameWidget> ActiveMinigameWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Cooking", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxIngredientOptionCount = 8;

	UPROPERTY(Transient)
	TMap<FName, FButtonStyle> OriginalIngredientSlotButtonStyles;

	bool bIngredientListVisible = false;
};
