// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
class USoundBase;
class UAudioComponent;

#include "TinoGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API ATinoGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ATinoGameMode();

	// 적이 플레이어를 인식하거나 놓칠 때 호출한다.
	void NotifyEnemyEngaged();
	void NotifyEnemyDisengaged();

protected:
	virtual void BeginPlay() override;

private:
	// 카운트에 맞춰 두 음악의 볼륨을 조절한다.
	void UpdateMusicState();

private:
	// 게임 내내 재생할 배경음악. 반복 여부는 사운드 애셋의 Looping 설정을 따른다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> BackgroundMusic;

	// 전투 중에 재생할 배경음악.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> CombatMusic;

	// 두 음악이 서로 바뀌는 데 걸리는 시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MusicFadeTime = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BaseMusicComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CombatMusicComponent;

	// 지금 플레이어를 쫓고 있는 적의 수.
	int32 EngagedEnemyCount = 0;

	// 직전에 전투 상태였는지. 적 수만 바뀌었을 때 음악을 다시 트는 것을 막는다.
	bool bWasInCombat = false;
};
