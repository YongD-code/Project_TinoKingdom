// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
class USoundBase;

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

protected:
	virtual void BeginPlay() override;

private:
	// 게임 내내 재생할 배경음악. 반복 여부는 사운드 애셋의 Looping 설정을 따른다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> BackgroundMusic;
};
