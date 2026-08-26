// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TinoPlayerWidget.generated.h"

enum class EExperienceBarState : uint8
{
	Idle,
	FillingToLevelUp,
	HoldingAtFull,
	MovingToFinalPercent
};

class AActor;
class UAbilitySystemComponent;
class UCanvasPanelSlot;
class UImage;
class UProgressBar;
class UPlayerProgressionComponent;
class UTextBlock;

struct FOnAttributeChangeData;

UCLASS()
class PROJECT_TINOKINGDOM_API UTinoPlayerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetCrosshairVisible(bool bVisible);
	void SetLockOnMarkerTarget(AActor* NewTarget);
	
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
	
private:
	bool BindToAbilitySystem();
	void UnbindFromAbilitySystem();
	
	bool BindToProgressionComponent();
	void UnbindFromProgressionComponent();
	
	void HandleHealthAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void HandleStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData);
	
	void HandleLevelChanged(int32 NewLevel);
	void HandleExperienceChanged(int32 NewExperience, int32 RequiredExperience);
	
	void RefreshHealthBar();
	void RefreshStaminaBar();
	void RefreshProgressionUI();
	
	void SetDisplayedLevel(int32 NewLevel);
	
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
	
	UPROPERTY(EditDefaultsOnly, Category = "UI|Experience", meta = (ClampMin = "0.0"))
	float LevelUpFullHoldDuration = 0.15;
	
	float TargetHealthPercent = 1.f;
	float TargetStaminaPercent = 1.f;
	float TargetExperiencePercent = 0.f;
	
	float FinalExperiencePercent = 0.f;
	float FullHoldElapsedTime = 0.f;
	
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
};
