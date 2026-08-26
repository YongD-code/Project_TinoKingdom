// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TinoPlayerWidget.generated.h"

class UImage;
class UCanvasPanelSlot;
class AActor;
class UProgressBar;
class UAbilitySystemComponent;

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
	
private:
	bool BindToAbilitySystem();
	void UnbindFromAbilitySystem();
	
	void HandleHealthAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void HandleStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData);
	
	void RefreshHealthBar();
	void RefreshStaminaBar();
	
	void UpdateLockOnMarkerPosition();
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanelSlot> LockOnMarkerSlot;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LockOnTarget;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;
	
	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;
	
	FDelegateHandle StaminaChangedDelegateHandle;
	FDelegateHandle MaxStaminaChangedDelegateHandle;
};
