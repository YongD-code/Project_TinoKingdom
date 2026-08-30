// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "TinoPlayerWidget.generated.h"

enum class EExperienceBarState : uint8
{
	Idle,
	FillingToLevelUp,
	HoldingAtFull,
	MovingToFinalPercent
};

enum class EPlayerStatType : uint8;

class AActor;
class UAbilitySystemComponent;
class UButton;
class UCanvasPanelSlot;
class UImage;
class UProgressBar;
class UPlayerProgressionComponent;
class USizeBox;
class UTextBlock;
class UUniformGridPanel;
class UWidget;
class UCookingWidget;

struct FOnAttributeChangeData;

UCLASS()
class PROJECT_TINOKINGDOM_API UTinoPlayerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetCrosshairVisible(bool bVisible);
	void SetLockOnMarkerTarget(AActor* NewTarget);
	
	void SetCharacterMenuVisible(bool bVisible);
	void ShowCookingIngredientPicker(UCookingWidget* CookingWidget, UInventoryComponent* InventoryComponent);
	void CloseCookingIngredientPicker();
	
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Crosshair;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LockOnMarker;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HPProgressBar;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> StaminaProgressBar;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelTextBlock;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ExperienceProgressBar;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatPointsTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MaxHealthValueTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MaxStaminaValueTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AttackPowerValueTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DefenseValueTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MaxHealthUpgradeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MaxStaminaUpgradeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AttackPowerUpgradeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DefenseUpgradeButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> InventoryPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> SlotGrid;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StatusPanel;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> HealthBarSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> StaminaBarSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> ExperienceBarSizeBox;
	
private:
	UFUNCTION()
	void HandleMaxHealthUpgradeClicked();

	UFUNCTION()
	void HandleMaxStaminaUpgradeClicked();

	UFUNCTION()
	void HandleAttackPowerUpgradeClicked();

	UFUNCTION()
	void HandleDefenseUpgradeClicked();

	UFUNCTION()
	void HandleInventorySlot0Clicked();

	UFUNCTION()
	void HandleInventorySlot1Clicked();

	UFUNCTION()
	void HandleInventorySlot2Clicked();

	UFUNCTION()
	void HandleInventorySlot3Clicked();

	UFUNCTION()
	void HandleInventorySlot4Clicked();

	UFUNCTION()
	void HandleInventorySlot5Clicked();

	UFUNCTION()
	void HandleInventorySlot6Clicked();

	UFUNCTION()
	void HandleInventorySlot7Clicked();

	UFUNCTION()
	void HandleInventorySlot8Clicked();

	UFUNCTION()
	void HandleInventorySlot9Clicked();

	UFUNCTION()
	void HandleInventorySlot10Clicked();

	UFUNCTION()
	void HandleInventorySlot11Clicked();

	UFUNCTION()
	void HandleInventorySlot12Clicked();

	UFUNCTION()
	void HandleInventorySlot13Clicked();

	UFUNCTION()
	void HandleInventorySlot14Clicked();

	UFUNCTION()
	void HandleInventorySlot15Clicked();
	
private:
	bool BindToAbilitySystem();
	void UnbindFromAbilitySystem();
	
	bool BindToProgressionComponent();
	void UnbindFromProgressionComponent();
	
	void HandleMaxHealthAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void HandleHealthAttributeChanged(const FOnAttributeChangeData& ChangeData);
	
	void HandleMaxStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void HandleStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData);
	
	void HandleStatAttributeChanged(const FOnAttributeChangeData& ChangeData);
	
	void HandleLevelChanged(int32 NewLevel);
	void HandleExperienceChanged(int32 NewExperience, int32 RequiredExperience);
	void HandleStatPointsChanged(int32 NewStatPoints);
	
	void RefreshHealthBar();
	void RefreshStaminaBar();
	void RefreshProgressionUI();
	void RefreshStatValue();
	
	void TryUpgradeStat(EPlayerStatType StatType);

	void RefreshInventorySlots();
	void HandleInventorySlotClicked(int32 SlotIndex);
	UInventoryComponent* ResolveInventoryComponent() const;
	
	void SetDisplayedLevel(int32 NewLevel);
	
	float CalculateBarWidth(float CurrentValue, float BaseValue, float WidthPerUnit) const;
	float CalculateExperiencePercent(int32 Experience, int32 RequiredExperience);
	
	void UpdateExperienceBar(float DeltaTime);
	void UpdateLockOnMarkerPosition();
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanelSlot> LockOnMarkerSlot;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LockOnTarget;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<UPlayerProgressionComponent> BoundProgressionComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Health", meta = (ClampMin = "0.0"))
	float HealthInterpSpeed = 8.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Stamina", meta = (ClampMin = "0.0"))
	float StaminaInterpSpeed = 8.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Experience", meta = (ClampMin = "0.0"))
	float ExperienceInterpSpeed = 2.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Bar Size", meta = (ClampMin = "1.0"))
	float BaseBarWidth = 393.9082f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Bar Size", meta = (ClampMin = "1.0"))
	float MaxBarWidth = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Bar Size|Health", meta = (ClampMin = "1.0"))
	float BaseMaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Bar Size|Health", meta = (ClampMin = "0.0"))
	float HealthBarWidthPerStatUnit = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Bar Size|Stamina", meta = (ClampMin = "1.0"))
	float BaseMaxStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Bar Size|Stamina", meta = (ClampMin = "0.0"))
	float StaminaBarWidthPerStatUnit = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Bar Size|Experience", meta = (ClampMin = "0.0"))
	float ExperienceBarWidthPerLevel = 2.f;
	
	float TargetHealthPercent = 1.f;
	float TargetStaminaPercent = 1.f;
	float TargetExperiencePercent = 0.f;
	
	float FinalExperiencePercent = 0.f;
	
	TArray<int32> PendingLevelUpLevels;
	
	EExperienceBarState ExperienceBarState = EExperienceBarState::Idle;
	
	bool bHPBarInitialized = false;
	bool bStaminaBarInitialized = false;
	bool bExperienceBarInitialized = false;
	bool bHasFinalExperiencePercent = false;
	
	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;
	
	FDelegateHandle StaminaChangedDelegateHandle;
	FDelegateHandle MaxStaminaChangedDelegateHandle;
	
	FDelegateHandle LevelChangedDelegateHandle;
	FDelegateHandle ExperienceChangedDelegateHandle;
	
	FDelegateHandle AttackPowerChangedDelegateHandle;
	FDelegateHandle DefenseChangedDelegateHandle;

	FDelegateHandle StatPointsChangedDelegateHandle;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> DisplayedInventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UCookingWidget> CookingIngredientTarget;

	UPROPERTY(Transient)
	TArray<FInventoryItemStack> DisplayedInventoryItems;

	static constexpr int32 InventoryColumnCount = 4;
	static constexpr int32 MaxInventorySlotCount = 16;

	bool bCookingIngredientPickerOpen = false;
};
