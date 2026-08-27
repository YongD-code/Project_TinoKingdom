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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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
	float TargetMoveSpeed = 0.45f;

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
	bool IsPlayerBarInTargetZone() const;

	bool bTargetMovingUp = true;
};
