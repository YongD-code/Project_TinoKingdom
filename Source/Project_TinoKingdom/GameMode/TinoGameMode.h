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

	// 사망 연출처럼 음악을 잠시 멈춰야 할 때 사용한다.
	void SetMusicSuspended(bool bSuspended);

	// 이 레벨에서 쓸 발소리. 애니메이션 노티파이가 읽어 간다.
	USoundBase* GetFootstepSound() const { return FootstepSound; }

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

	// 이 레벨의 발소리. 레벨마다 다른 소리를 쓸 수 있도록 GameMode가 들고 있는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> FootstepSound;

	// 두 음악이 서로 바뀌는 데 걸리는 시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MusicFadeTime = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BaseMusicComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CombatMusicComponent;

	// 지금 플레이어를 쫓고 있는 적의 수.
	int32 EngagedEnemyCount = 0;

	// 사망 연출 등으로 음악을 멈춘 상태인지.
	bool bMusicSuspended = false;

	// 직전에 각 곡이 켜져 있었는지. BeginPlay에서 기본 곡을 틀어두므로 그 상태로 시작한다.
	bool bBaseWasOn = true;
	bool bCombatWasOn = false;
};
