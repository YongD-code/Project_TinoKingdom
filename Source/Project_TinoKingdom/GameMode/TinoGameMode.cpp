// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoGameMode.h"

#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Player/TinoPlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

ATinoGameMode::ATinoGameMode()
{
	PlayerControllerClass = ATinoPlayerController::StaticClass();
	DefaultPawnClass = APlayerCharacter::StaticClass();
}

void ATinoGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 두 곡을 함께 틀어두고 볼륨만 바꾼다. 전환할 때 곡이 처음부터 다시 시작하지 않는다.
	// bAutoDestroy를 끄지 않으면 재생이 끝날 때 컴포넌트가 사라져 볼륨을 조절할 수 없다.
	if (IsValid(BackgroundMusic))
	{
		BaseMusicComponent = UGameplayStatics::SpawnSound2D(
			this, BackgroundMusic, 1.0f, 1.0f, 0.0f, nullptr, false, false);
	}

	if (IsValid(CombatMusic))
	{
		CombatMusicComponent = UGameplayStatics::SpawnSound2D(
			this, CombatMusic, 1.0f, 1.0f, 0.0f, nullptr, false, false);

		// 전투가 시작될 때 FadeIn으로 켜므로 처음에는 멈춰 둔다.
		CombatMusicComponent->Stop();
	}
}

void ATinoGameMode::NotifyEnemyEngaged()
{
	++EngagedEnemyCount;
	UpdateMusicState();
}

void ATinoGameMode::NotifyEnemyDisengaged()
{
	// 죽거나 레벨이 바뀌는 도중에 중복으로 불려도 음수로 내려가지 않게 막는다.
	EngagedEnemyCount = FMath::Max(0, EngagedEnemyCount - 1);
	UpdateMusicState();
}

void ATinoGameMode::UpdateMusicState()
{
	const bool bInCombat = EngagedEnemyCount > 0;

	// 적 수만 바뀌고 전투 여부는 그대로면 지금 나오는 곡을 그대로 둔다.
	if (bWasInCombat == bInCombat)
	{
		return;
	}

	bWasInCombat = bInCombat;

	// AdjustVolume은 볼륨만 건드려 정지 상태를 되살리지 못한다.
	// FadeIn은 재생을 시작하고 FadeOut은 끝나면 정지시키므로 전환에 이쪽을 쓴다.
	if (IsValid(BaseMusicComponent))
	{
		if (bInCombat)
		{
			BaseMusicComponent->FadeOut(MusicFadeTime, 0.0f);
		}
		else
		{
			BaseMusicComponent->FadeIn(MusicFadeTime, 1.0f);
		}
	}

	if (IsValid(CombatMusicComponent))
	{
		if (bInCombat)
		{
			CombatMusicComponent->FadeIn(MusicFadeTime, 1.0f);
		}
		else
		{
			CombatMusicComponent->FadeOut(MusicFadeTime, 0.0f);
		}
	}
}
