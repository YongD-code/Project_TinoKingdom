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
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Project_TinoKingdom/Component/CookingRecipeBookComponent.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Component/PlayerProgressionComponent.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAttributeSet.h"
#include "Project_TinoKingdom/Interface/TargetableInterface.h"
#include "Project_TinoKingdom/UI/CookingWidget.h"

namespace
{
FString GetCookingQualityDisplayString(ECookingQuality Quality)
{
	switch (Quality)
	{
	case ECookingQuality::Failed:
		return TEXT("Failed");
	case ECookingQuality::Normal:
		return TEXT("Normal");
	case ECookingQuality::Good:
		return TEXT("Good");
	case ECookingQuality::Special:
		return TEXT("Special");
	default:
		return TEXT("Unknown");
	}
}

FLinearColor GetCookingQualityDisplayColor(ECookingQuality Quality)
{
	switch (Quality)
	{
	case ECookingQuality::Failed:
		return FLinearColor(0.82f, 0.28f, 0.22f, 1.0f);
	case ECookingQuality::Normal:
		return FLinearColor(0.86f, 0.80f, 0.68f, 1.0f);
	case ECookingQuality::Good:
		return FLinearColor(0.55f, 0.82f, 1.0f, 1.0f);
	case ECookingQuality::Special:
		return FLinearColor(1.0f, 0.78f, 0.28f, 1.0f);
	default:
		return FLinearColor::White;
	}
}

FString GetCookingResultTypeDisplayString(ECookingResultType ResultType)
{
	switch (ResultType)
	{
	case ECookingResultType::Jelly:
		return TEXT("젤리");
	case ECookingResultType::Soup:
		return TEXT("수프");
	case ECookingResultType::Grill:
		return TEXT("구이");
	case ECookingResultType::Failed:
		return TEXT("실패");
	default:
		return TEXT("요리");
	}
}
}

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
		EnsureRecipeBookWidget();
		RefreshRecipeBookPanel();
	}
	else
	{
		CloseCookingIngredientPicker();
		HideInventoryItemPreview();
		HideRecipeBookPanel();
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
		InventoryPanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		InventoryPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		InventoryPanelSlot->SetPosition(FVector2D(-600.0f, 0.0f));
		InventoryPanelSlot->SetSize(FVector2D(360.0f, 360.0f));
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

		if (IsInViewport())
		{
			RemoveFromParent();
			AddToViewport();
		}
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

FReply UTinoPlayerWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && TryUseInventoryFoodAt(HoveredInventorySlotIndex))
	{
		return FReply::Handled();
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
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

#define DEFINE_INVENTORY_SLOT_HOVERED(SlotIndex) \
void UTinoPlayerWidget::HandleInventorySlot##SlotIndex##Hovered() \
{ \
	HandleInventorySlotHovered(SlotIndex); \
}

DEFINE_INVENTORY_SLOT_HOVERED(0)
DEFINE_INVENTORY_SLOT_HOVERED(1)
DEFINE_INVENTORY_SLOT_HOVERED(2)
DEFINE_INVENTORY_SLOT_HOVERED(3)
DEFINE_INVENTORY_SLOT_HOVERED(4)
DEFINE_INVENTORY_SLOT_HOVERED(5)
DEFINE_INVENTORY_SLOT_HOVERED(6)
DEFINE_INVENTORY_SLOT_HOVERED(7)
DEFINE_INVENTORY_SLOT_HOVERED(8)
DEFINE_INVENTORY_SLOT_HOVERED(9)
DEFINE_INVENTORY_SLOT_HOVERED(10)
DEFINE_INVENTORY_SLOT_HOVERED(11)
DEFINE_INVENTORY_SLOT_HOVERED(12)
DEFINE_INVENTORY_SLOT_HOVERED(13)
DEFINE_INVENTORY_SLOT_HOVERED(14)
DEFINE_INVENTORY_SLOT_HOVERED(15)
DEFINE_INVENTORY_SLOT_HOVERED(16)
DEFINE_INVENTORY_SLOT_HOVERED(17)
DEFINE_INVENTORY_SLOT_HOVERED(18)
DEFINE_INVENTORY_SLOT_HOVERED(19)
DEFINE_INVENTORY_SLOT_HOVERED(20)
DEFINE_INVENTORY_SLOT_HOVERED(21)
DEFINE_INVENTORY_SLOT_HOVERED(22)
DEFINE_INVENTORY_SLOT_HOVERED(23)
DEFINE_INVENTORY_SLOT_HOVERED(24)

#undef DEFINE_INVENTORY_SLOT_HOVERED

void UTinoPlayerWidget::HandleInventorySlotUnhovered()
{
	HoveredInventorySlotIndex = INDEX_NONE;
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
	StatPointsTextBlock->SetText(
		FText::Format(NSLOCTEXT("TinoPlayerWidget", "StatPointsTextFormat", "Stat Points {0}"),
			FText::AsNumber(NewStatPoints)));
	
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
			FInventoryItemStack DisplayItem = Item;
			if (bCookingIngredientPickerOpen && CookingIngredientTarget != nullptr)
			{
				DisplayItem.Count -= CookingIngredientTarget->GetSelectedIngredientCountForItem(DisplayItem.ItemId);
			}

			if (DisplayItem.Count <= 0)
			{
				continue;
			}

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
				SlotWidget->SetToolTip(BuildInventoryItemToolTipWidget(DisplayedInventoryItems[Index], Index));
			}
			else
			{
				SlotWidget->SetToolTip(nullptr);
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

			switch (Index)
			{
			case 0:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot0Hovered);
				break;
			case 1:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot1Hovered);
				break;
			case 2:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot2Hovered);
				break;
			case 3:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot3Hovered);
				break;
			case 4:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot4Hovered);
				break;
			case 5:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot5Hovered);
				break;
			case 6:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot6Hovered);
				break;
			case 7:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot7Hovered);
				break;
			case 8:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot8Hovered);
				break;
			case 9:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot9Hovered);
				break;
			case 10:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot10Hovered);
				break;
			case 11:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot11Hovered);
				break;
			case 12:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot12Hovered);
				break;
			case 13:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot13Hovered);
				break;
			case 14:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot14Hovered);
				break;
			case 15:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot15Hovered);
				break;
			case 16:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot16Hovered);
				break;
			case 17:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot17Hovered);
				break;
			case 18:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot18Hovered);
				break;
			case 19:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot19Hovered);
				break;
			case 20:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot20Hovered);
				break;
			case 21:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot21Hovered);
				break;
			case 22:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot22Hovered);
				break;
			case 23:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot23Hovered);
				break;
			case 24:
				SlotButton->OnHovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlot24Hovered);
				break;
			default:
				break;
			}

			SlotButton->OnUnhovered.AddUniqueDynamic(this, &UTinoPlayerWidget::HandleInventorySlotUnhovered);
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

	if (bCookingIngredientPickerOpen)
	{
		if (CookingIngredientTarget == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cooking ingredient picker is open, but CookingIngredientTarget is null."));
			return;
		}

		if (CookingIngredientTarget->AddIngredientFromInventory(DisplayedInventoryItems[SlotIndex]))
		{
			RefreshInventorySlots();
			if (CookingIngredientTarget != nullptr && CookingIngredientTarget->IsIngredientSelectionFull())
			{
				CloseCookingIngredientPicker();
			}
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

UCookingRecipeBookComponent* UTinoPlayerWidget::ResolveCookingRecipeBookComponent() const
{
	const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	return PlayerCharacter != nullptr ? PlayerCharacter->GetCookingRecipeBookComponent() : nullptr;
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
		PreviewSlot->SetAnchors(FAnchors(1.0f, 0.5f, 1.0f, 0.5f));
		PreviewSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PreviewSlot->SetPosition(FVector2D(-210.0f, -2.0f));
		PreviewSlot->SetSize(FVector2D(360.0f, 360.0f));
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

void UTinoPlayerWidget::EnsureRecipeBookWidget()
{
	if (RecipeBookPanel != nullptr && RecipeBookListBox != nullptr && RecipeBookCountTextBlock != nullptr)
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

	RecipeBookPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RecipeBookPanel_Runtime"));
	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RecipeBookRoot_Runtime"));
	UTextBlock* TitleTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecipeBookTitle_Runtime"));
	RecipeBookCountTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecipeBookCount_Runtime"));
	RecipeBookScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RecipeBookScroll_Runtime"));
	RecipeBookListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RecipeBookList_Runtime"));

	if (RecipeBookPanel == nullptr || RootBox == nullptr || TitleTextBlock == nullptr ||
		RecipeBookCountTextBlock == nullptr || RecipeBookScrollBox == nullptr || RecipeBookListBox == nullptr)
	{
		return;
	}

	FSlateFontInfo TitleFont = TitleTextBlock->GetFont();
	TitleFont.Size = 30;
	TitleTextBlock->SetFont(TitleFont);
	TitleTextBlock->SetText(FText::FromString(TEXT("요리 도감")));
	TitleTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.86f, 0.56f, 1.0f)));

	FSlateFontInfo CountFont = RecipeBookCountTextBlock->GetFont();
	CountFont.Size = 18;
	RecipeBookCountTextBlock->SetFont(CountFont);
	RecipeBookCountTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.76f, 0.72f, 0.64f, 1.0f)));

	RecipeBookPanel->SetBrushColor(FLinearColor(0.025f, 0.021f, 0.016f, 0.92f));
	RecipeBookPanel->SetPadding(FMargin(18.0f, 16.0f));
	RecipeBookPanel->SetContent(RootBox);
	RecipeBookPanel->SetVisibility(ESlateVisibility::Collapsed);

	UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleTextBlock);
	if (TitleSlot != nullptr)
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UVerticalBoxSlot* CountSlot = RootBox->AddChildToVerticalBox(RecipeBookCountTextBlock);
	if (CountSlot != nullptr)
	{
		CountSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	RecipeBookScrollBox->AddChild(RecipeBookListBox);
	UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(RecipeBookScrollBox);
	if (ScrollSlot != nullptr)
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UCanvasPanelSlot* RecipeBookSlot = RootCanvas->AddChildToCanvas(RecipeBookPanel);
	if (RecipeBookSlot != nullptr)
	{
		RecipeBookSlot->SetAnchors(FAnchors(1.0f, 0.5f, 1.0f, 0.5f));
		RecipeBookSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		RecipeBookSlot->SetPosition(FVector2D(-245.0f, -2.0f));
		RecipeBookSlot->SetSize(FVector2D(430.0f, 620.0f));
		RecipeBookSlot->SetZOrder(18);
	}
}

void UTinoPlayerWidget::RefreshRecipeBookPanel()
{
	EnsureRecipeBookWidget();
	if (RecipeBookPanel == nullptr || RecipeBookListBox == nullptr || RecipeBookCountTextBlock == nullptr)
	{
		return;
	}

	RecipeBookListBox->ClearChildren();

	const UCookingRecipeBookComponent* RecipeBookComponent = ResolveCookingRecipeBookComponent();
	const TArray<FDiscoveredCookingRecipe>& Recipes =
		RecipeBookComponent != nullptr ? RecipeBookComponent->GetDiscoveredRecipes() : TArray<FDiscoveredCookingRecipe>();

	RecipeBookCountTextBlock->SetText(FText::Format(
		FText::FromString(TEXT("발견한 요리 {0}개")),
		FText::AsNumber(Recipes.Num())
	));

	if (Recipes.IsEmpty())
	{
		UTextBlock* EmptyTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecipeBookEmpty_Runtime"));
		if (EmptyTextBlock != nullptr)
		{
			FSlateFontInfo EmptyFont = EmptyTextBlock->GetFont();
			EmptyFont.Size = 22;
			EmptyTextBlock->SetFont(EmptyFont);
			EmptyTextBlock->SetText(FText::FromString(TEXT("아직 발견한 요리가 없습니다.")));
			EmptyTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.78f, 0.68f, 1.0f)));
			EmptyTextBlock->SetAutoWrapText(true);
			RecipeBookListBox->AddChildToVerticalBox(EmptyTextBlock);
		}
		RecipeBookPanel->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	for (const FDiscoveredCookingRecipe& Recipe : Recipes)
	{
		if (UWidget* RowWidget = BuildRecipeBookRow(Recipe))
		{
			UVerticalBoxSlot* RowSlot = RecipeBookListBox->AddChildToVerticalBox(RowWidget);
			if (RowSlot != nullptr)
			{
				RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
			}
		}
	}

	RecipeBookPanel->SetVisibility(ESlateVisibility::Visible);
}

void UTinoPlayerWidget::HideRecipeBookPanel()
{
	if (RecipeBookPanel != nullptr)
	{
		RecipeBookPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWidget* UTinoPlayerWidget::BuildRecipeBookRow(const FDiscoveredCookingRecipe& Recipe)
{
	if (WidgetTree == nullptr)
	{
		return nullptr;
	}

	UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* IconSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UImage* IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	UTextBlock* RowTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (RowBorder == nullptr || RowBox == nullptr || IconSizeBox == nullptr || IconImage == nullptr || RowTextBlock == nullptr)
	{
		return nullptr;
	}

	RowBorder->SetBrushColor(FLinearColor(0.095f, 0.075f, 0.045f, 0.74f));
	RowBorder->SetPadding(FMargin(10.0f));
	RowBorder->SetContent(RowBox);

	IconSizeBox->SetWidthOverride(72.0f);
	IconSizeBox->SetHeightOverride(72.0f);
	if (Recipe.Icon != nullptr)
	{
		IconImage->SetBrushFromTexture(Recipe.Icon, true);
	}
	IconSizeBox->AddChild(IconImage);

	UHorizontalBoxSlot* IconSlot = RowBox->AddChildToHorizontalBox(IconSizeBox);
	if (IconSlot != nullptr)
	{
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	FSlateFontInfo RowFont = RowTextBlock->GetFont();
	RowFont.Size = 19;
	RowTextBlock->SetFont(RowFont);
	RowTextBlock->SetText(BuildRecipeBookRowText(Recipe));
	RowTextBlock->SetColorAndOpacity(FSlateColor(GetCookingQualityDisplayColor(Recipe.BestQuality)));
	RowTextBlock->SetAutoWrapText(true);

	UHorizontalBoxSlot* TextSlot = RowBox->AddChildToHorizontalBox(RowTextBlock);
	if (TextSlot != nullptr)
	{
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	return RowBorder;
}

FText UTinoPlayerWidget::BuildRecipeBookRowText(const FDiscoveredCookingRecipe& Recipe) const
{
	FString Text = FString::Printf(
		TEXT("%s\n%s / 최고 %s / %d회"),
		*Recipe.ResultName.ToString(),
		*GetCookingResultTypeDisplayString(Recipe.ResultType),
		*GetCookingQualityDisplayString(Recipe.BestQuality),
		Recipe.TimesCooked
	);

	if (!FMath::IsNearlyZero(Recipe.BestHealAmount))
	{
		Text += FString::Printf(TEXT("\n체력 %+d"), FMath::RoundToInt(Recipe.BestHealAmount));
	}
	if (Recipe.BestStaminaAmount > 0.0f)
	{
		Text += FString::Printf(TEXT("  스태미나 +%d"), FMath::RoundToInt(Recipe.BestStaminaAmount));
	}
	if (Recipe.BestAttackBuffAmount > 0.0f)
	{
		Text += FString::Printf(TEXT("  공격 +%d"), FMath::RoundToInt(Recipe.BestAttackBuffAmount));
	}
	if (Recipe.BestDefenseBuffAmount > 0.0f)
	{
		Text += FString::Printf(TEXT("  방어 +%d"), FMath::RoundToInt(Recipe.BestDefenseBuffAmount));
	}

	return FText::FromString(Text);
}

FText UTinoPlayerWidget::BuildInventoryItemToolTipText(const FInventoryItemStack& Item) const
{
	FString ToolTip = FString::Printf(TEXT("%s x%d"), *Item.DisplayName.ToString(), Item.Count);

	if (Item.ItemType == EInventoryItemType::Food)
	{
		const FCookingResultData& FoodData = Item.FoodResultData;
		ToolTip += FString::Printf(TEXT("\n품질: %s"), *GetCookingQualityDisplayString(FoodData.Quality));

		if (FoodData.Quality == ECookingQuality::Failed || FoodData.ResultType == ECookingResultType::Failed)
		{
			ToolTip += TEXT("\n완전히 실패한 음식 .. 먹으면 배탈이 날것 같다");
			ToolTip += TEXT("\n우클릭: 먹기");
			return FText::FromString(ToolTip);
		}

		if (FoodData.HealAmount > 0.0f)
		{
			ToolTip += FString::Printf(TEXT("\n체력 회복 +%d"), FMath::RoundToInt(FoodData.HealAmount));
		}
		if (FoodData.StaminaAmount > 0.0f)
		{
			ToolTip += FString::Printf(TEXT("\n스태미나 회복 +%d"), FMath::RoundToInt(FoodData.StaminaAmount));
		}
		if (FoodData.AttackBuffAmount > 0.0f)
		{
			ToolTip += FString::Printf(TEXT("\n공격력 +%d"), FMath::RoundToInt(FoodData.AttackBuffAmount));
		}
		if (FoodData.DefenseBuffAmount > 0.0f)
		{
			ToolTip += FString::Printf(TEXT("\n방어력 +%d"), FMath::RoundToInt(FoodData.DefenseBuffAmount));
		}

		ToolTip += TEXT("\n우클릭: 먹기");
	}
	else if (Item.ItemType == EInventoryItemType::Key)
	{
		ToolTip += TEXT("\n어딘가의 잠긴 길을 여는 열쇠입니다.");
	}

	return FText::FromString(ToolTip);
}

UWidget* UTinoPlayerWidget::BuildInventoryItemToolTipWidget(const FInventoryItemStack& Item, int32 SlotIndex)
{
	if (WidgetTree == nullptr)
	{
		return nullptr;
	}

	UBorder* ToolTipBorder = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass()
	);
	UTextBlock* ToolTipTextBlock = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass()
	);

	if (ToolTipBorder == nullptr || ToolTipTextBlock == nullptr)
	{
		return nullptr;
	}

	FSlateFontInfo ToolTipFont = ToolTipTextBlock->GetFont();
	ToolTipFont.Size = 26;
	ToolTipTextBlock->SetFont(ToolTipFont);
	ToolTipTextBlock->SetText(BuildInventoryItemToolTipText(Item));
	ToolTipTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ToolTipTextBlock->SetAutoWrapText(true);
	ToolTipTextBlock->SetMinDesiredWidth(280.0f);

	ToolTipBorder->SetBrushColor(FLinearColor(0.02f, 0.018f, 0.014f, 0.96f));
	ToolTipBorder->SetPadding(FMargin(18.0f, 14.0f));
	ToolTipBorder->SetContent(ToolTipTextBlock);

	return ToolTipBorder;
}

void UTinoPlayerWidget::HandleInventorySlotHovered(int32 SlotIndex)
{
	HoveredInventorySlotIndex = SlotIndex;
}

bool UTinoPlayerWidget::TryUseInventoryFoodAt(int32 SlotIndex)
{
	if (bCookingIngredientPickerOpen || !DisplayedInventoryItems.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FInventoryItemStack Item = DisplayedInventoryItems[SlotIndex];
	if (Item.ItemType != EInventoryItemType::Food)
	{
		return false;
	}

	UInventoryComponent* InventoryComponent = DisplayedInventoryComponent.Get();
	if (InventoryComponent == nullptr)
	{
		InventoryComponent = ResolveInventoryComponent();
	}
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	bool bUsed = ApplyFoodEffectsToAbilitySystem(Item.FoodResultData);
	if (!bUsed)
	{
		if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
		{
			bUsed = InventoryComponent->UseFoodItem(Item.ItemId, PlayerCharacter->FindComponentByClass<UStatComponent>());
		}
	}
	else
	{
		bUsed = InventoryComponent->RemoveItem(Item.ItemId, 1);
	}

	if (bUsed)
	{
		DisplayedInventoryComponent = InventoryComponent;
		RefreshInventorySlots();
		HideInventoryItemPreview();
		HoveredInventorySlotIndex = INDEX_NONE;
	}

	return bUsed;
}

bool UTinoPlayerWidget::ApplyFoodEffectsToAbilitySystem(const FCookingResultData& FoodData)
{
	UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (AbilitySystemComponent == nullptr)
	{
		return false;
	}

	bool bApplied = false;

	if (!FMath::IsNearlyZero(FoodData.HealAmount))
	{
		const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(UTinoAttributeSet::GetMaxHealthAttribute());
		const float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UTinoAttributeSet::GetHealthAttribute());
		AbilitySystemComponent->SetNumericAttributeBase(
			UTinoAttributeSet::GetHealthAttribute(),
			FMath::Clamp(CurrentHealth + FoodData.HealAmount, 0.0f, MaxHealth)
		);
		bApplied = true;
	}

	if (FoodData.StaminaAmount > 0.0f)
	{
		const float MaxStamina = AbilitySystemComponent->GetNumericAttribute(UTinoAttributeSet::GetMaxStaminaAttribute());
		const float CurrentStamina = AbilitySystemComponent->GetNumericAttribute(UTinoAttributeSet::GetStaminaAttribute());
		AbilitySystemComponent->SetNumericAttributeBase(
			UTinoAttributeSet::GetStaminaAttribute(),
			FMath::Clamp(CurrentStamina + FoodData.StaminaAmount, 0.0f, MaxStamina)
		);
		bApplied = true;
	}

	if (FoodData.AttackBuffAmount > 0.0f)
	{
		const float CurrentAttackPower = AbilitySystemComponent->GetNumericAttribute(UTinoAttributeSet::GetAttackPowerAttribute());
		AbilitySystemComponent->SetNumericAttributeBase(
			UTinoAttributeSet::GetAttackPowerAttribute(),
			CurrentAttackPower + FoodData.AttackBuffAmount
		);
		bApplied = true;
	}

	if (FoodData.DefenseBuffAmount > 0.0f)
	{
		const float CurrentDefense = AbilitySystemComponent->GetNumericAttribute(UTinoAttributeSet::GetDefenseAttribute());
		AbilitySystemComponent->SetNumericAttributeBase(
			UTinoAttributeSet::GetDefenseAttribute(),
			CurrentDefense + FoodData.DefenseBuffAmount
		);
		bApplied = true;
	}

	return bApplied;
}

void UTinoPlayerWidget::SetDisplayedLevel(int32 NewLevel)
{
	LevelTextBlock->SetText(
		FText::Format(NSLOCTEXT("TinoPlayerWidget", "LevelTextFormat", "Level {0}"), 
		FText::AsNumber(NewLevel)));
	
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
