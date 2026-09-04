// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CookingMinigameWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingMinigameFinished, float, FinalScore);

UCLASS()
class PROJECT_TINOKINGDOM_API UCookingMinigameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cooking|Minigame")
	void StartCookingMinigame();

	UFUNCTION(BlueprintCallable, Category = "Cooking|Minigame")
	void SetCookingInputHeld(bool bHeld);

	UFUNCTION(BlueprintCallable, Category = "Cooking|Minigame")
	void FinishCookingMinigame();

	UFUNCTION(BlueprintPure, Category = "Cooking|Minigame")
	float GetPlayerBarPosition() const { return PlayerBarPosition; }

	UFUNCTION(BlueprintPure, Category = "Cooking|Minigame")
	float GetTargetPosition() const { return TargetPosition; }

	UFUNCTION(BlueprintPure, Category = "Cooking|Minigame")
	float GetMinigameScore() const { return MinigameScore; }

	UFUNCTION(BlueprintPure, Category = "Cooking|Minigame")
	float GetRemainingTime() const;

	UPROPERTY(BlueprintAssignable, Category = "Cooking|Minigame")
	FOnCookingMinigameFinished OnCookingMinigameFinished;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Cooking|Minigame")
	void OnMinigameUpdated(
		float NewPlayerBarPosition,
		float NewTargetPosition,
		float NewScore,
		float NewRemainingTime
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cooking|Minigame")
	void OnMinigameEnded(float FinalScore);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "1.0"))
	float Duration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.0"))
	float TargetMoveSpeed = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.1"))
	float TargetDirectionChangeMinTime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.1"))
	float TargetDirectionChangeMaxTime = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float TargetZoneSize = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.0"))
	float PlayerBarRiseSpeed = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.0"))
	float PlayerBarFallSpeed = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.0"))
	float ScoreIncreasePerSecond = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.0"))
	float ScoreDecreasePerSecond = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cooking|Minigame")
	bool bMinigamePlaying = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cooking|Minigame")
	bool bCookingInputHeld = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cooking|Minigame")
	float PlayerBarPosition = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Cooking|Minigame")
	float TargetPosition = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Cooking|Minigame")
	float MinigameScore = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cooking|Minigame")
	float ElapsedTime = 0.0f;

private:
	void BuildDefaultMinigameVisuals();
	void RefreshDefaultMinigameVisuals();
	void ResetTargetDirectionTimer();
	bool IsPlayerBarInTargetZone() const;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> InstructionTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> ScoreTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> TargetLabelTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> PlayerLabelTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> TimeLabelTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> QualityTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> GaugeCanvas;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> GaugeTrackBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> GaugeSuccessFillBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> TargetZoneBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> PlayerNeedleBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> TargetMarkerTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> PlayerMarkerTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<class UProgressBar> TimeProgressBar;

	float TargetDirection = 1.0f;
	float TargetDirectionChangeTimer = 0.0f;

	static constexpr float GaugeWidth = 980.0f;
	static constexpr float GaugeHeight = 74.0f;
};
