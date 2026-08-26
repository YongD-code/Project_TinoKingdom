// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoPlayerWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Component/PlayerProgressionComponent.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAttributeSet.h"
#include "Project_TinoKingdom/Interface/TargetableInterface.h"

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

void UTinoPlayerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	LockOnMarkerSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(LockOnMarker);
	
	SetCrosshairVisible(false);
	SetLockOnMarkerTarget(nullptr);
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
	if (bExperienceBarInitialized)
	{
		const float DisplayPercent = ExperienceProgressBar->GetPercent();
		const float NewPercent = FMath::FInterpTo(
			DisplayPercent, TargetExperiencePercent, InDeltaTime, ExperienceInterpSpeed);
		ExperienceProgressBar->SetPercent(NewPercent);
	}
	if (!LockOnTarget.IsValid())
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	UpdateLockOnMarkerPosition();
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
			this, &UTinoPlayerWidget::HandleHealthAttributeChanged);
	
	StaminaChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UTinoAttributeSet::GetStaminaAttribute()).AddUObject(
			this, &UTinoPlayerWidget::HandleStaminaAttributeChanged);
	MaxStaminaChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UTinoAttributeSet::GetMaxStaminaAttribute()).AddUObject(
			this, &UTinoPlayerWidget::HandleStaminaAttributeChanged);
	
	RefreshHealthBar();
	RefreshStaminaBar();
	
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
	}
	
	HealthChangedDelegateHandle.Reset();
	MaxHealthChangedDelegateHandle.Reset();
	
	StaminaChangedDelegateHandle.Reset();
	MaxStaminaChangedDelegateHandle.Reset();
	
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
	}
	
	LevelChangedDelegateHandle.Reset();
	ExperienceChangedDelegateHandle.Reset();
	
	bExperienceBarInitialized = false;
	TargetExperiencePercent = 0.f;
	
	BoundProgressionComponent.Reset();
}

void UTinoPlayerWidget::HandleHealthAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshHealthBar();
}

void UTinoPlayerWidget::HandleStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshStaminaBar();
}

void UTinoPlayerWidget::HandleLevelChanged(int32 NewLevel)
{
	LevelTextBlock->SetText(FText::AsNumber(NewLevel));
}

void UTinoPlayerWidget::HandleExperienceChanged(int32 NewExperience, int32 RequiredExperience)
{
	const UPlayerProgressionComponent* ProgressionComponent = BoundProgressionComponent.Get();
	
	float ExperiencePercent = 0.f;
	if (ProgressionComponent != nullptr && ProgressionComponent->IsMaxLevel())
	{
		ExperiencePercent = 1.f;
	}
	else if (RequiredExperience > 0.f)
	{
		ExperiencePercent = FMath::Clamp(
			static_cast<float>(NewExperience) / static_cast<float>(RequiredExperience), 0.f, 1.f);
	}

	TargetExperiencePercent = ExperiencePercent;
	if (!bExperienceBarInitialized)
	{
		ExperienceProgressBar->SetPercent(TargetExperiencePercent);
		bExperienceBarInitialized = true;
	}
}

void UTinoPlayerWidget::RefreshHealthBar()
{
	UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (AbilitySystemComponent == nullptr || HPProgressBar == nullptr)
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
	if (AbilitySystemComponent == nullptr || StaminaProgressBar == nullptr)
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
	
	HandleLevelChanged(ProgressionComponent->GetCurrentLevel());
	HandleExperienceChanged(ProgressionComponent->GetCurrentExperience(), 
		ProgressionComponent->GetRequiredExperienceForNextLevel());
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
