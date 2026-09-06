// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeathScreenWidget.generated.h"

class UTextBlock;
class UBorder;
class USoundBase;

UCLASS()
class PROJECT_TINOKINGDOM_API UDeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowDeathMessage(AActor* DamageCauser);
	void FadeToBlack(float Duration);
	void HideDeathMessage();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeathMessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DeathFadeBorder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Screen", meta = (ClampMin = "0.0"))
	float TextFadeInDuration = 0.45f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> DeathFadeSound;

private:
	void BuildDefaultVisuals();
	FText MakeDeathMessage(AActor* DamageCauser) const;

	double FadeStartRealTime = 0.0;
	float FadeToBlackDuration = 0.0f;
	bool bFadingTextIn = false;
	bool bFadingToBlack = false;
};
