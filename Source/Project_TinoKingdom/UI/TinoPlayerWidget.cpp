// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoPlayerWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Pawn.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Component/PlayerProgressionComponent.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAttributeSet.h"
#include "Project_TinoKingdom/Interface/TargetableInterface.h"
#include "Project_TinoKingdom/UI/CookingWidget.h"

void UTinoPlayerWidget::SetCrosshairVisible(bool bVisible)
{
	Crosshair->SetVisibility((bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed));
}

void UTinoPlayerWidget::SetLockOnMarkerTarget(AActor* NewTarget)
{
	LockOnTarget = NewTarget;
	if (!LockOnTarget.IsValid())
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	UpdateLockOnMarkerPosition();
}

void UTinoPlayerWidget::SetCharacterMenuVisible(bool bVisible)
{
	if (!ensureMsgf(InventoryPanel != nullptr && StatusPanel != nullptr, TEXT("InventoryPanel or StatusPanel 이름 불일치")))
	{
		return;
	}

	if (bVisible)
	{
		bCookingIngredientPickerOpen = false;
		CookingIngredientTarget = nullptr;
		DisplayedInventoryComponent = ResolveInventoryComponent();
		EnsureInventoryPreviewWidget();

		if (UCanvasPanelSlot* InventoryPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(InventoryPanel))
		{
			InventoryPanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			InventoryPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			InventoryPanelSlot->SetPosition(FVector2D(-242.960938f, -2.540527f));
			InventoryPanelSlot->SetSize(FVector2D(700.0f, 700.0f));
		}

		RefreshInventorySlots();
	}
	else
	{
		CloseCookingIngredientPicker();
		HideInventoryItemPreview();
	}

	const ESlateVisibility MenuVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	InventoryPanel->SetVisibility(MenuVisibility);
	StatusPanel->SetVisibility(MenuVisibility);
}

void UTinoPlayerWidget::ShowCookingIngredientPicker(UCookingWidget* CookingWidget, UInventoryComponent* InventoryComponent)
{
	if (!ensureMsgf(InventoryPanel != nullptr && StatusPanel != nullptr, TEXT("InventoryPanel or StatusPanel 이름 불일치")))
	{
		return;
	}

	bCookingIngredientPickerOpen = true;
	CookingIngredientTarget = CookingWidget;
	DisplayedInventoryComponent = InventoryComponent != nullptr ? InventoryComponent : ResolveInventoryComponent();

	InventoryPanel->SetVisibility(ESlateVisibility::Visible);
	StatusPanel->SetVisibility(ESlateVisibility::Collapsed);
	HideInventoryItemPreview();

	if (UCanvasPanelSlot* InventoryPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(InventoryPanel))
	{
		InventoryPanelSlot->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
		InventoryPanelSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		InventoryPanelSlot->SetPosition(FVector2D(40.0f, 0.0f));
		InventoryPanelSlot->SetSize(FVector2D(520.0f, 520.0f));
	}

	RefreshInventorySlots();
}

void UTinoPlayerWidget::RefreshCookingIngredientPicker()
{
	if (bCookingIngredientPickerOpen)
	{
		RefreshInventorySlots();
	}
}

void UTinoPlayerWidget::CloseCookingIngredientPicker()
{
	if (bCookingIngredientPickerOpen)
	{
		bCookingIngredientPickerOpen = false;
		CookingIngredientTarget = nullptr;
		if (InventoryPanel != nullptr)
		{
			InventoryPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		HideInventoryItemPreview();
	}
}

void UTinoPlayerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	LockOnMarkerSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(LockOnMarker);
	
	SetCrosshairVisible(false);
	SetLockOnMarkerTarget(nullptr);
	SetCharacterMenuVisible(false);
	
	MaxHealthUpgradeButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleMaxHealthUpgradeClicked);
	MaxStaminaUpgradeButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleMaxStaminaUpgradeClicked);
	AttackPowerUpgradeButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleAttackPowerUpgradeClicked);
	DefenseUpgradeButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleDefenseUpgradeClicked);
}

void UTinoPlayerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindToAbilitySystem();
	BindToProgressionComponent();
}

void UTinoPlayerWidget::NativeDestruct()
{
	UnbindFromAbilitySystem();
	UnbindFromProgressionComponent();
	Super::NativeDestruct();
}

void UTinoPlayerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (bHPBarInitialized)
	{
		const float DisplayPercent = HPProgressBar->GetPercent();
		const float NewPercent = FMath::FInterpTo(
			DisplayPercent, TargetHealthPercent, InDeltaTime, HealthInterpSpeed);
		HPProgressBar->SetPercent(NewPercent);
	}
	if (bStaminaBarInitialized)
	{
		const float DisplayPercent = StaminaProgressBar->GetPercent();
		const float NewPercent = FMath::FInterpTo(
			DisplayPercent, TargetStaminaPercent, InDeltaTime, StaminaInterpSpeed);
		StaminaProgressBar->SetPercent(NewPercent);
	}

	UpdateExperienceBar(InDeltaTime);
	
	if (!LockOnTarget.IsValid())
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	UpdateLockOnMarkerPosition();
}

void UTinoPlayerWidget::HandleMaxHealthUpgradeClicked()
{
	TryUpgradeStat(EPlayerStatType::MaxHealth);
}

void UTinoPlayerWidget::HandleMaxStaminaUpgradeClicked()
{
	TryUpgradeStat(EPlayerStatType::MaxStamina);
}

void UTinoPlayerWidget::HandleAttackPowerUpgradeClicked()
{
	TryUpgradeStat(EPlayerStatType::AttackPower);
}

void UTinoPlayerWidget::HandleDefenseUpgradeClicked()
{
	TryUpgradeStat(EPlayerStatType::Defense);
}

void UTinoPlayerWidget::HandleInventorySlot0Clicked()
{
	HandleInventorySlotClicked(0);
}

void UTinoPlayerWidget::HandleInventorySlot1Clicked()
{
	HandleInventorySlotClicked(1);
}

void UTinoPlayerWidget::HandleInventorySlot2Clicked()
{
	HandleInventorySlotClicked(2);
}

void UTinoPlayerWidget::HandleInventorySlot3Clicked()
{
	HandleInventorySlotClicked(3);
}

void UTinoPlayerWidget::HandleInventorySlot4Clicked()
{
	HandleInventorySlotClicked(4);
}

void UTinoPlayerWidget::HandleInventorySlot5Clicked()
{
	HandleInventorySlotClicked(5);
}

void UTinoPlayerWidget::HandleInventorySlot6Clicked()
{
	HandleInventorySlotClicked(6);
}

void UTinoPlayerWidget::HandleInventorySlot7Clicked()
{
	HandleInventorySlotClicked(7);
}

void UTinoPlayerWidget::HandleInventorySlot8Clicked()
{
	HandleInventorySlotClicked(8);
}

void UTinoPlayerWidget::HandleInventorySlot9Clicked()
{
	HandleInventorySlotClicked(9);
}

void UTinoPlayerWidget::HandleInventorySlot10Clicked()
{
	HandleInventorySlotClicked(10);
}

void UTinoPlayerWidget::HandleInventorySlot11Clicked()
{
	HandleInventorySlotClicked(11);
}

void UTinoPlayerWidget::HandleInventorySlot12Clicked()
{
	HandleInventorySlotClicked(12);
}

void UTinoPlayerWidget::HandleInventorySlot13Clicked()
{
	HandleInventorySlotClicked(13);
}

void UTinoPlayerWidget::HandleInventorySlot14Clicked()
{
	HandleInventorySlotClicked(14);
}

void UTinoPlayerWidget::HandleInventorySlot15Clicked()
{
	HandleInventorySlotClicked(15);
}

void UTinoPlayerWidget::HandleInventorySlot16Clicked()
{
	HandleInventorySlotClicked(16);
}

void UTinoPlayerWidget::HandleInventorySlot17Clicked()
{
	HandleInventorySlotClicked(17);
}

void UTinoPlayerWidget::HandleInventorySlot18Clicked()
{
	HandleInventorySlotClicked(18);
}

void UTinoPlayerWidget::HandleInventorySlot19Clicked()
{
	HandleInventorySlotClicked(19);
}

void UTinoPlayerWidget::HandleInventorySlot20Clicked()
{
	HandleInventorySlotClicked(20);
}

void UTinoPlayerWidget::HandleInventorySlot21Clicked()
{
	HandleInventorySlotClicked(21);
}

void UTinoPlayerWidget::HandleInventorySlot22Clicked()
{
	HandleInventorySlotClicked(22);
}

void UTinoPlayerWidget::HandleInventorySlot23Clicked()
{
	HandleInventorySlotClicked(23);
}

void UTinoPlayerWidget::HandleInventorySlot24Clicked()
{
	HandleInventorySlotClicked(24);
}

bool UTinoPlayerWidget::BindToAbilitySystem()
{
	UnbindFromAbilitySystem();
	if (!ensureMsgf(HPProgressBar != nullptr, TEXT("PlayerUI의 HPProgressBar와 이름 불일치")))
	{
		return false;
	}
	
	APawn* PlayerPawn = GetOwningPlayerPawn();
	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(PlayerPawn);
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
	const UTinoAttributeSet* AttributeSet = AbilitySystemComponent->GetSet<UTinoAttributeSet>();
	
	BoundAbilitySystemComponent = AbilitySystemComponent;
	
	HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UTinoAttributeSet::GetHealthAttribute()).AddUObject(
			this, &UTinoPlayerWidget::HandleHealthAttributeChanged);
	MaxHealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UTinoAttributeSet::GetMaxHealthAttribute()).AddUObject(
			this, &UTinoPlayerWidget::HandleMaxHealthAttributeChanged);
	
	StaminaChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UTinoAttributeSet::GetStaminaAttribute()).AddUObject(
			this, &UTinoPlayerWidget::HandleStaminaAttributeChanged);
	MaxStaminaChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UTinoAttributeSet::GetMaxStaminaAttribute()).AddUObject(
			this, &UTinoPlayerWidget::HandleMaxStaminaAttributeChanged);
	
	AttackPowerChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
	UTinoAttributeSet::GetAttackPowerAttribute()).AddUObject(
		this, &UTinoPlayerWidget::HandleStatAttributeChanged);

	DefenseChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UTinoAttributeSet::GetDefenseAttribute()).AddUObject(
			this, &UTinoPlayerWidget::HandleStatAttributeChanged);
	
	RefreshHealthBar();
	RefreshStaminaBar();
	RefreshStatValue();
	
	return true;
}

void UTinoPlayerWidget::UnbindFromAbilitySystem()
{
	UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (AbilitySystemComponent != nullptr)
	{
		if (HealthChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UTinoAttributeSet::GetHealthAttribute()).Remove(
					HealthChangedDelegateHandle);
		}
		if (MaxHealthChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UTinoAttributeSet::GetMaxHealthAttribute()).Remove(
					MaxHealthChangedDelegateHandle);
		}
		
		if (StaminaChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UTinoAttributeSet::GetStaminaAttribute()).Remove(
					StaminaChangedDelegateHandle);
		}
		if (MaxStaminaChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UTinoAttributeSet::GetMaxStaminaAttribute()).Remove(
					MaxStaminaChangedDelegateHandle);
		}
		
		if (AttackPowerChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UTinoAttributeSet::GetAttackPowerAttribute()).Remove(
					AttackPowerChangedDelegateHandle);
		}
		if (DefenseChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				UTinoAttributeSet::GetDefenseAttribute()).Remove(
					DefenseChangedDelegateHandle);
		}
	}
	
	HealthChangedDelegateHandle.Reset();
	MaxHealthChangedDelegateHandle.Reset();
	
	StaminaChangedDelegateHandle.Reset();
	MaxStaminaChangedDelegateHandle.Reset();
	
	AttackPowerChangedDelegateHandle.Reset();
	DefenseChangedDelegateHandle.Reset();
	
	bHPBarInitialized = false;
	bStaminaBarInitialized = false;
	BoundAbilitySystemComponent.Reset();
}

bool UTinoPlayerWidget::BindToProgressionComponent()
{
	UnbindFromProgressionComponent();
	
	if (!ensureMsgf(LevelTextBlock != nullptr, TEXT("PlayerUI에 LevelTextBlock이 없거나 이름이 일치하지 않습니다.")
	))
	{
		return false;
	}

	if (!ensureMsgf(ExperienceProgressBar != nullptr, TEXT("PlayerUI에 ExperienceProgressBar가 없거나 이름이 일치하지 않습니다.")))
	{
		return false;
	}
	
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	UPlayerProgressionComponent* ProgressionComponent = PlayerCharacter->GetProgressionComponent();
	BoundProgressionComponent = ProgressionComponent;
	
	LevelChangedDelegateHandle = ProgressionComponent->OnLevelChanged.AddUObject(this, 
		&UTinoPlayerWidget::HandleLevelChanged);
	ExperienceChangedDelegateHandle = ProgressionComponent->OnExperienceChanged.AddUObject(this,
		&UTinoPlayerWidget::HandleExperienceChanged);
	
	StatPointsChangedDelegateHandle = ProgressionComponent->OnStatPointsChanged.AddUObject(this,
		&UTinoPlayerWidget::HandleStatPointsChanged);
	
	RefreshProgressionUI();
	return true;
}

void UTinoPlayerWidget::UnbindFromProgressionComponent()
{
	UPlayerProgressionComponent* ProgressionComponent = BoundProgressionComponent.Get();
	if (ProgressionComponent != nullptr)
	{
		if (LevelChangedDelegateHandle.IsValid())
		{
			ProgressionComponent->OnLevelChanged.Remove(LevelChangedDelegateHandle);
		}
		if (ExperienceChangedDelegateHandle.IsValid())
		{
			ProgressionComponent->OnExperienceChanged.Remove(ExperienceChangedDelegateHandle);
		}
		if (StatPointsChangedDelegateHandle.IsValid())
		{
			ProgressionComponent->OnStatPointsChanged.Remove(StatPointsChangedDelegateHandle);
		}
	}
	
	LevelChangedDelegateHandle.Reset();
	ExperienceChangedDelegateHandle.Reset();
	StatPointsChangedDelegateHandle.Reset();
	
	PendingLevelUpLevels.Reset();
	ExperienceBarState = EExperienceBarState::Idle;
	
	bExperienceBarInitialized = false;
	bHasFinalExperiencePercent = false;
	
	TargetExperiencePercent = 0.f;
	FinalExperiencePercent = 0.f;
	
	BoundProgressionComponent.Reset();
}

void UTinoPlayerWidget::HandleMaxHealthAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshHealthBar();
	RefreshStatValue();
}

void UTinoPlayerWidget::HandleHealthAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshHealthBar();
}

void UTinoPlayerWidget::HandleMaxStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshStaminaBar();
	RefreshStatValue();
}

void UTinoPlayerWidget::HandleStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshStaminaBar();
}

void UTinoPlayerWidget::HandleStatAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshStatValue();
}

void UTinoPlayerWidget::HandleLevelChanged(int32 NewLevel)
{
	if (!bExperienceBarInitialized)
	{
		SetDisplayedLevel(NewLevel);
		return;
	}
	
	PendingLevelUpLevels.Add(NewLevel);
	
	if (ExperienceBarState == EExperienceBarState::Idle || 
		ExperienceBarState == EExperienceBarState::MovingToFinalPercent)
	{
		TargetExperiencePercent = 1.f;
		ExperienceBarState = EExperienceBarState::FillingToLevelUp;
	}
}

void UTinoPlayerWidget::HandleExperienceChanged(int32 NewExperience, int32 RequiredExperience)
{
	FinalExperiencePercent = CalculateExperiencePercent(NewExperience, RequiredExperience);
	
	bHasFinalExperiencePercent = true;
	
	if (!bExperienceBarInitialized)
	{
		TargetExperiencePercent = FinalExperiencePercent;
		ExperienceProgressBar->SetPercent(TargetExperiencePercent);

		bExperienceBarInitialized = true;
		bHasFinalExperiencePercent = false;
		return;
	}
	
	if (ExperienceBarState == EExperienceBarState::FillingToLevelUp ||
		ExperienceBarState == EExperienceBarState::HoldingAtFull)
	{
		return;
	}
	
	if (!PendingLevelUpLevels.IsEmpty())
	{
		TargetExperiencePercent = 1.f;
		ExperienceBarState = EExperienceBarState::FillingToLevelUp;
		
		return;
	}
	
	TargetExperiencePercent = FinalExperiencePercent;
	ExperienceBarState = EExperienceBarState::MovingToFinalPercent;
}

void UTinoPlayerWidget::HandleStatPointsChanged(int32 NewStatPoints)
{
	StatPointsTextBlock->SetText(FText::AsNumber(NewStatPoints));
	
	const bool bCanUpgrade = NewStatPoints > 0;
	
	MaxHealthUpgradeButton->SetIsEnabled(bCanUpgrade);
	MaxStaminaUpgradeButton->SetIsEnabled(bCanUpgrade);
	AttackPowerUpgradeButton->SetIsEnabled(bCanUpgrade);
	DefenseUpgradeButton->SetIsEnabled(bCanUpgrade);
}

void UTinoPlayerWidget::RefreshHealthBar()
{
	UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (AbilitySystemComponent == nullptr || HPProgressBar == nullptr || HealthBarSizeBox == nullptr)
	{
		return;
	}
	
	const UTinoAttributeSet* AttributeSet = AbilitySystemComponent->GetSet<UTinoAttributeSet>();
	if (AttributeSet == nullptr)
	{
		return;
	}
	
	const float CurrentHealth = AttributeSet->GetHealth();
	const float MaxHealth = AttributeSet->GetMaxHealth();
	
	HealthBarSizeBox->SetWidthOverride(CalculateBarWidth(MaxHealth, BaseMaxHealth, HealthBarWidthPerStatUnit));
	const float HealthPercent = MaxHealth > 0.f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f) : 0.f;

	TargetHealthPercent = HealthPercent;
	if (!bHPBarInitialized)
	{
		HPProgressBar->SetPercent(TargetHealthPercent);
		bHPBarInitialized = true;
	}
}

void UTinoPlayerWidget::RefreshStaminaBar()
{
	UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (AbilitySystemComponent == nullptr || StaminaProgressBar == nullptr || StaminaBarSizeBox == nullptr)
	{
		return;
	}
	
	const UTinoAttributeSet* AttributeSet = AbilitySystemComponent->GetSet<UTinoAttributeSet>();
	if (AttributeSet == nullptr)
	{
		return;
	}
	
	const float CurrentStamina = AttributeSet->GetStamina();
	const float MaxStamina = AttributeSet->GetMaxStamina();
	
	StaminaBarSizeBox->SetWidthOverride(CalculateBarWidth(MaxStamina, BaseMaxStamina, StaminaBarWidthPerStatUnit));
	const float StaminaPercent = MaxStamina > 0.f ? FMath::Clamp(CurrentStamina / MaxStamina, 0.f, 1.f) : 0.f;
	
	TargetStaminaPercent = StaminaPercent;
	if (!bStaminaBarInitialized)
	{
		StaminaProgressBar->SetPercent(TargetStaminaPercent);
		bStaminaBarInitialized = true;
	}
}

void UTinoPlayerWidget::RefreshProgressionUI()
{
	const UPlayerProgressionComponent* ProgressionComponent = BoundProgressionComponent.Get();
	
	PendingLevelUpLevels.Reset();
	
	ExperienceBarState = EExperienceBarState::Idle;
	
	bHasFinalExperiencePercent = false;
	
	SetDisplayedLevel(ProgressionComponent->GetCurrentLevel());
	FinalExperiencePercent = CalculateExperiencePercent(
		ProgressionComponent->GetCurrentExperience(),
		ProgressionComponent->GetRequiredExperienceForNextLevel());
	
	TargetExperiencePercent = FinalExperiencePercent;
	ExperienceProgressBar->SetPercent(TargetExperiencePercent);
	
	bExperienceBarInitialized = true;
	
	HandleStatPointsChanged(ProgressionComponent->GetUnspentStatPoints());
}

void UTinoPlayerWidget::RefreshStatValue()
{
	UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	const UTinoAttributeSet* AttributeSet = AbilitySystemComponent->GetSet<UTinoAttributeSet>();
	
	MaxHealthValueTextBlock->SetText(
	FText::Format(NSLOCTEXT("TinoPlayerWidget", "HealthValueFormat", "Health {0}"),
		FText::AsNumber(FMath::RoundToInt(AttributeSet->GetMaxHealth()))));

	MaxStaminaValueTextBlock->SetText(
		FText::Format(NSLOCTEXT("TinoPlayerWidget", "StaminaValueFormat", "Stamina {0}"),
			FText::AsNumber(FMath::RoundToInt(AttributeSet->GetMaxStamina()))));

	AttackPowerValueTextBlock->SetText(
		FText::Format(NSLOCTEXT("TinoPlayerWidget", "AttackValueFormat", "Attack {0}"),
			FText::AsNumber(FMath::RoundToInt(AttributeSet->GetAttackPower()))));

	DefenseValueTextBlock->SetText(
		FText::Format(NSLOCTEXT("TinoPlayerWidget", "DefenseValueFormat", "Defense {0}"),
			FText::AsNumber(FMath::RoundToInt(AttributeSet->GetDefense()))));
}

void UTinoPlayerWidget::TryUpgradeStat(EPlayerStatType StatType)
{
	UPlayerProgressionComponent* ProgressionComponent = BoundProgressionComponent.Get();
	ProgressionComponent->TryUpgradeStat(StatType);
}

void UTinoPlayerWidget::RefreshInventorySlots()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	UUniformGridPanel* SlotGrid = WidgetTree->FindWidget<UUniformGridPanel>(TEXT("SlotGrid"));
	if (SlotGrid == nullptr)
	{
		return;
	}

	SlotGrid->ClearChildren();
	DisplayedInventoryItems.Empty();
	DisplayedInventorySlots.Empty();

	if (DisplayedInventoryComponent != nullptr)
	{
		for (const FInventoryItemStack& Item : DisplayedInventoryComponent->GetItems())
		{
			int32 DisplayCount = Item.Count;

			if (bCookingIngredientPickerOpen && CookingIngredientTarget != nullptr)
			{
				for (const FInventoryItemStack& SelectedIngredient : CookingIngredientTarget->GetSelectedIngredients())
				{
					if (SelectedIngredient.ItemId == Item.ItemId)
					{
						--DisplayCount;
					}
				}
			}

			if (DisplayCount <= 0)
			{
				continue;
			}

			FInventoryItemStack DisplayItem = Item;
			DisplayItem.Count = DisplayCount;
			DisplayedInventoryItems.Add(DisplayItem);
			if (DisplayedInventoryItems.Num() >= MaxInventorySlotCount)
			{
				break;
			}
		}
	}

	if (InventorySlotWidgetClass == nullptr)
	{
		InventorySlotWidgetClass = LoadClass<UUserWidget>(
			nullptr,
			TEXT("/Game/UI/InventoryUI.InventoryUI_C")
		);
	}

	for (int32 Index = 0; Index < MaxInventorySlotCount; ++Index)
	{
		UUserWidget* SlotWidget = nullptr;
		if (InventorySlotWidgetClass != nullptr)
		{
			SlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), InventorySlotWidgetClass);
		}

		if (SlotWidget == nullptr)
		{
			continue;
		}

		if (UFunction* SetSlotItemFunction = SlotWidget->FindFunction(TEXT("SetSlotItem")))
		{
			struct FSetSlotItemParams
			{
				UTexture2D* ItemIcon = nullptr;
				int32 ItemCount = 0;
			};

			FSetSlotItemParams Params;
			if (DisplayedInventoryItems.IsValidIndex(Index))
			{
				Params.ItemIcon = DisplayedInventoryItems[Index].Icon;
				Params.ItemCount = DisplayedInventoryItems[Index].Count;
			}
			SlotWidget->ProcessEvent(SetSlotItemFunction, &Params);
		}

		if (UButton* SlotButton = Cast<UButton>(SlotWidget->GetWidgetFromName(TEXT("ButtonSlot"))))
		{
			SlotButton->SetIsEnabled(DisplayedInventoryItems.IsValidIndex(Index));

			switch (Index)
			{
			case 0:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot0Clicked);
				break;
			case 1:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot1Clicked);
				break;
			case 2:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot2Clicked);
				break;
			case 3:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot3Clicked);
				break;
			case 4:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot4Clicked);
				break;
			case 5:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot5Clicked);
				break;
			case 6:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot6Clicked);
				break;
			case 7:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot7Clicked);
				break;
			case 8:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot8Clicked);
				break;
			case 9:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot9Clicked);
				break;
			case 10:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot10Clicked);
				break;
			case 11:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot11Clicked);
				break;
			case 12:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot12Clicked);
				break;
			case 13:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot13Clicked);
				break;
			case 14:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot14Clicked);
				break;
			case 15:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot15Clicked);
				break;
			case 16:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot16Clicked);
				break;
			case 17:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot17Clicked);
				break;
			case 18:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot18Clicked);
				break;
			case 19:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot19Clicked);
				break;
			case 20:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot20Clicked);
				break;
			case 21:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot21Clicked);
				break;
			case 22:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot22Clicked);
				break;
			case 23:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot23Clicked);
				break;
			case 24:
				SlotButton->OnClicked.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot24Clicked);
				break;
			default:
				break;
			}
		}

		UUniformGridSlot* GridSlot = SlotGrid->AddChildToUniformGrid(
			SlotWidget,
			Index / InventoryColumnCount,
			Index % InventoryColumnCount
		);
		if (GridSlot != nullptr)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}

		DisplayedInventorySlots.Add(SlotWidget);
	}
}

void UTinoPlayerWidget::HandleInventorySlotClicked(int32 SlotIndex)
{
	if (!DisplayedInventoryItems.IsValidIndex(SlotIndex))
	{
		HideInventoryItemPreview();
		return;
	}

	if (bCookingIngredientPickerOpen && CookingIngredientTarget != nullptr)
	{
		if (CookingIngredientTarget->AddIngredientFromInventory(DisplayedInventoryItems[SlotIndex]))
		{
			RefreshInventorySlots();
		}
		return;
	}

	ShowInventoryItemPreview(DisplayedInventoryItems[SlotIndex]);
}

UInventoryComponent* UTinoPlayerWidget::ResolveInventoryComponent() const
{
	const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	return PlayerCharacter != nullptr ? PlayerCharacter->GetInventoryComponent() : nullptr;
}

void UTinoPlayerWidget::EnsureInventoryPreviewWidget()
{
	if (InventoryPreviewPanel != nullptr && InventoryPreviewImage != nullptr)
	{
		return;
	}

	if (WidgetTree == nullptr)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (RootCanvas == nullptr)
	{
		return;
	}

	InventoryPreviewPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryPreviewPanel_Runtime"));
	InventoryPreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InventoryPreviewImage_Runtime"));
	if (InventoryPreviewPanel == nullptr || InventoryPreviewImage == nullptr)
	{
		return;
	}

	InventoryPreviewPanel->SetBrushColor(FLinearColor(0.02f, 0.018f, 0.015f, 0.86f));
	InventoryPreviewPanel->SetPadding(FMargin(18.0f));
	InventoryPreviewPanel->SetContent(InventoryPreviewImage);
	InventoryPreviewPanel->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* PreviewSlot = RootCanvas->AddChildToCanvas(InventoryPreviewPanel);
	if (PreviewSlot != nullptr)
	{
		PreviewSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PreviewSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PreviewSlot->SetPosition(FVector2D(360.0f, -2.0f));
		PreviewSlot->SetSize(FVector2D(300.0f, 300.0f));
		PreviewSlot->SetZOrder(20);
	}
}

void UTinoPlayerWidget::ShowInventoryItemPreview(const FInventoryItemStack& Item)
{
	EnsureInventoryPreviewWidget();
	if (InventoryPreviewPanel == nullptr || InventoryPreviewImage == nullptr || Item.Icon == nullptr)
	{
		HideInventoryItemPreview();
		return;
	}

	InventoryPreviewImage->SetBrushFromTexture(Item.Icon, true);
	InventoryPreviewPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTinoPlayerWidget::HideInventoryItemPreview()
{
	if (InventoryPreviewPanel != nullptr)
	{
		InventoryPreviewPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTinoPlayerWidget::SetDisplayedLevel(int32 NewLevel)
{
	LevelTextBlock->SetText(FText::AsNumber(NewLevel));
	
	ExperienceBarSizeBox->SetWidthOverride(
		CalculateBarWidth(static_cast<float>(NewLevel), 1.f, ExperienceBarWidthPerLevel));
}

float UTinoPlayerWidget::CalculateBarWidth(float CurrentValue, float BaseValue, float WidthPerUnit) const
{
	const float SafeMaxBarWidth = FMath::Max(MaxBarWidth, BaseBarWidth);
	const float GrowthValue = FMath::Max(CurrentValue - BaseValue, 0.f);
	const float GrowthWidth = GrowthValue * FMath::Max(WidthPerUnit, 0.f);
	
	return FMath::Clamp(BaseBarWidth + GrowthWidth, BaseBarWidth, SafeMaxBarWidth);
}

float UTinoPlayerWidget::CalculateExperiencePercent(int32 Experience, int32 RequiredExperience)
{
	const UPlayerProgressionComponent* ProgressionComponent = BoundProgressionComponent.Get();
	if (ProgressionComponent->IsMaxLevel())
	{
		return 1.f;
	}
	
	if (RequiredExperience <= 0)
	{
		return 0.f;
	}
	
	// 실제 경험치는 0이지만 어색함을 줄이기 위해 화면에는 1%로 표시한다.
	if (Experience <= 0)
	{
		return 0.01f;
	}
	
	return FMath::Clamp(static_cast<float>(Experience) / static_cast<float>(RequiredExperience), 0.f, 1.f);
}

void UTinoPlayerWidget::UpdateExperienceBar(float DeltaTime)
{
	if (!bExperienceBarInitialized)
	{
		return;
	}
	if (ExperienceBarState == EExperienceBarState::Idle)
	{
		return;
	}
	
	if (ExperienceBarState == EExperienceBarState::HoldingAtFull)
	{
		// FillingToLevelUp에서 100%를 설정한 다음 Tick에 이 상태를 처리한다.
		// 따라서 별도 타이머 없이도 100%가 최소 한 프레임 렌더링된다.
		// 레벨업 후 실제 경험치는 0이지만 화면에는 1% 표시
		ExperienceProgressBar->SetPercent(0.01f);
		
		if (!PendingLevelUpLevels.IsEmpty())
		{
			TargetExperiencePercent = 1.f;
			ExperienceBarState = EExperienceBarState::FillingToLevelUp;
		}
		else if (bHasFinalExperiencePercent)
		{
			TargetExperiencePercent = FinalExperiencePercent;
			ExperienceBarState = EExperienceBarState::MovingToFinalPercent;
		}
		else
		{
			TargetExperiencePercent = 0.01f;
			ExperienceBarState = EExperienceBarState::Idle;
		}
		
		return;
	}
	
	const float DisplayPercent = ExperienceProgressBar->GetPercent();
	const float NewPercent = FMath::FInterpTo(
		DisplayPercent, TargetExperiencePercent, DeltaTime, ExperienceInterpSpeed);
	
	constexpr float CompletionTolerance = 0.001f;
	if (!FMath::IsNearlyEqual(NewPercent, TargetExperiencePercent, CompletionTolerance))
	{
		ExperienceProgressBar->SetPercent(NewPercent);
		return;
	}
	
	ExperienceProgressBar->SetPercent(TargetExperiencePercent);
	
	if (ExperienceBarState == EExperienceBarState::MovingToFinalPercent)
	{
		ExperienceBarState = EExperienceBarState::Idle;
		bHasFinalExperiencePercent = false;
		return;
	}
	
	if (ExperienceBarState != EExperienceBarState::FillingToLevelUp)
	{
		return;
	}
	if (PendingLevelUpLevels.IsEmpty())
	{
		if (bHasFinalExperiencePercent)
		{
			TargetExperiencePercent = FinalExperiencePercent;
			ExperienceBarState = EExperienceBarState::MovingToFinalPercent;
		}
		else
		{
			ExperienceBarState = EExperienceBarState::Idle;
		}
		return;
	}
	
	const int32 NewLevel = PendingLevelUpLevels[0];
	PendingLevelUpLevels.RemoveAt(0);

	SetDisplayedLevel(NewLevel);

	if (NewLevel >= UPlayerProgressionComponent::MaxLevel)
	{
		PendingLevelUpLevels.Reset();

		FinalExperiencePercent = 1.f;
		TargetExperiencePercent = 1.f;

		bHasFinalExperiencePercent = false;
		ExperienceBarState = EExperienceBarState::Idle;

		return;
	}

	ExperienceBarState = EExperienceBarState::HoldingAtFull;
}

void UTinoPlayerWidget::UpdateLockOnMarkerPosition()
{
	AActor* TargetActor = LockOnTarget.Get();
	const FVector LockOnLocation = ITargetableInterface::Execute_GetLockOnLocation(TargetActor);
	
	FVector2D MarkerPosition;
	const bool bProjected = UWidgetLayoutLibrary::
		ProjectWorldLocationToWidgetPosition(
			GetOwningPlayer(),
			LockOnLocation,
			MarkerPosition,
			true
		);
	
	if (!bProjected)
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	LockOnMarkerSlot->SetPosition(MarkerPosition);
	LockOnMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
}
