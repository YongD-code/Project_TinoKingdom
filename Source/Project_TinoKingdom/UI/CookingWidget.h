// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Styling/SlateTypes.h"
#include "CookingWidget.generated.h"

class UCookingComponent;
class UButton;
class UCookingMinigameWidget;
class UEditableTextBox;
class UImage;
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

	UFUNCTION(BlueprintPure, Category = "Cooking")
	bool IsIngredientSelectionFull() const;

	int32 GetSelectedIngredientCountForItem(FName ItemId) const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Cooking")
	void OnSelectedIngredientsChanged(const TArray<FInventoryItemStack>& SelectedIngredients);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cooking")
	void OnCookingCompleted(const FCookingResultData& ResultData);

private:
	void NormalizeCookingWidgetLayering();
	void ResetCookingSelection();
	void BroadcastSelectedIngredientsChanged();
	TArray<FInventoryItemStack> GetCompactSelectedIngredients() const;
	void UpdateIngredientSlotTexts(const TArray<FInventoryItemStack>& SelectedIngredients);
	void UpdateIngredientSlotImages(const TArray<FInventoryItemStack>& SelectedIngredients);
	void SetIngredientSlotText(int32 Index, const FText& Text);
	void SetIngredientSlotImage(int32 Index, UTexture2D* Icon);
	void CacheIngredientSlotButtonStyles();
	UButton* FindIngredientSlotButton(int32 Index) const;
	UTextBlock* FindIngredientSlotTextBlock(int32 Index) const;
	void HandleIngredientSlotClicked(int32 Index);
	bool AddIngredientToSlot(const FInventoryItemStack& Ingredient, int32 SlotIndex);
	bool HasIngredientInSlot(int32 SlotIndex) const;
	int32 FindFirstEmptyIngredientSlot() const;
	void SyncCookingComponentFromSlots();
	void SetResultText(const FText& Text);
	void OpenIngredientPicker();
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
	void HandleIngredientSlot0Clicked();

	UFUNCTION()
	void HandleIngredientSlot1Clicked();

	UFUNCTION()
	void HandleIngredientSlot2Clicked();

	UFUNCTION()
	void HandleIngredientSlot3Clicked();

	UPROPERTY(Transient)
	TObjectPtr<UCookingComponent> CookingComponent;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_StartCooking;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> M1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> M2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> M3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> M4;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseCookingButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> TextBox_Result;

	UPROPERTY(EditDefaultsOnly, Category = "Cooking", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float DefaultCookingScore = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cooking|Minigame")
	TSubclassOf<UCookingMinigameWidget> CookingMinigameWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UCookingMinigameWidget> ActiveMinigameWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> IngredientSlotTextBlocks;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> IngredientSlotImages;

	TArray<FButtonStyle> IngredientSlotButtonStyles;

	UPROPERTY(Transient)
	TArray<FInventoryItemStack> IngredientSlots;

	uint64 LastIngredientSlotClickFrame = 0;
	int32 LastIngredientSlotClickIndex = INDEX_NONE;
	int32 PendingIngredientSlotIndex = INDEX_NONE;
};
