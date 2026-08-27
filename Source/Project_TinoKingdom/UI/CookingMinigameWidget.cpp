// Fill out your copyright notice in the Description page of Project Settings.

#include "CookingMinigameWidget.h"

void UCookingMinigameWidget::StartCookingMinigame()
{
	bMinigamePlaying = true;
	bCookingInputHeld = false;
	bTargetMovingUp = true;

	ElapsedTime = 0.0f;
	MinigameScore = 50.0f;
	PlayerBarPosition = 0.5f;
	TargetPosition = 0.5f;

	OnMinigameUpdated(PlayerBarPosition, TargetPosition, MinigameScore, GetRemainingTime());
}

void UCookingMinigameWidget::SetCookingInputHeld(bool bHeld)
{
	bCookingInputHeld = bHeld;
}

void UCookingMinigameWidget::FinishCookingMinigame()
{
	if (!bMinigamePlaying)
	{
		return;
	}

	bMinigamePlaying = false;

	OnCookingMinigameFinished.Broadcast(MinigameScore);
	OnMinigameEnded(MinigameScore);
}

float UCookingMinigameWidget::GetRemainingTime() const
{
	return FMath::Max(Duration - ElapsedTime, 0.0f);
}

void UCookingMinigameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bMinigamePlaying)
	{
		return;
	}

	ElapsedTime += InDeltaTime;

	const float TargetDirection = bTargetMovingUp ? 1.0f : -1.0f;
	TargetPosition += TargetDirection * TargetMoveSpeed * InDeltaTime;

	if (TargetPosition >= 1.0f)
	{
		TargetPosition = 1.0f;
		bTargetMovingUp = false;
	}
	else if (TargetPosition <= 0.0f)
	{
		TargetPosition = 0.0f;
		bTargetMovingUp = true;
	}

	const float PlayerDirection = bCookingInputHeld ? 1.0f : -1.0f;
	const float PlayerSpeed = bCookingInputHeld ? PlayerBarRiseSpeed : PlayerBarFallSpeed;
	PlayerBarPosition = FMath::Clamp(PlayerBarPosition + PlayerDirection * PlayerSpeed * InDeltaTime, 0.0f, 1.0f);

	const float ScoreDelta = IsPlayerBarInTargetZone() ? ScoreIncreasePerSecond : -ScoreDecreasePerSecond;
	MinigameScore = FMath::Clamp(MinigameScore + ScoreDelta * InDeltaTime, 0.0f, 100.0f);

	OnMinigameUpdated(PlayerBarPosition, TargetPosition, MinigameScore, GetRemainingTime());

	if (ElapsedTime >= Duration)
	{
		FinishCookingMinigame();
	}
}

bool UCookingMinigameWidget::IsPlayerBarInTargetZone() const
{
	const float HalfTargetSize = TargetZoneSize * 0.5f;
	return FMath::Abs(PlayerBarPosition - TargetPosition) <= HalfTargetSize;
}
