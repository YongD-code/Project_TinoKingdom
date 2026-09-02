// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"
#include "Project_TinoKingdom/DataAsset/QuestData.h"
#include "TinoGameInstance.generated.h"

class APlayerCharacter;
class UEquipmentLoadoutData;

// OpenLevel은 기존 World와 PlayerCharacter를 파괴한다. 다음 World에서 새로 생성되는
// PlayerCharacter에 복원할 수 있도록 World에 종속되지 않는 값만 보관한다.
USTRUCT()
struct PROJECT_TINOKINGDOM_API FTinoPlayerTravelState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Level = 1;

	UPROPERTY()
	int32 Experience = 0;

	UPROPERTY()
	int32 UnspentStatPoints = 0;

	UPROPERTY()
	float Health = 0.f;

	UPROPERTY()
	float MaxHealth = 1.f;

	UPROPERTY()
	float Stamina = 0.f;

	UPROPERTY()
	float MaxStamina = 0.f;

	UPROPERTY()
	float AttackPower = 0.f;

	UPROPERTY()
	float Defense = 0.f;

	UPROPERTY()
	TArray<FInventoryItemStack> InventoryItems;

	UPROPERTY()
	TMap<TObjectPtr<UQuestData>, EQuestState> QuestStates;

	UPROPERTY()
	TObjectPtr<UQuestData> TrackedQuest;

	UPROPERTY()
	TObjectPtr<UEquipmentLoadoutData> EquipmentLoadout;
};

UCLASS()
class PROJECT_TINOKINGDOM_API UTinoGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 현재 PlayerCharacter의 런타임 진행 정보를 다음 맵에 넘길 대기 상태로 저장한다.
	UFUNCTION(BlueprintCallable, Category = "Player Travel")
	bool CapturePlayerState(APlayerCharacter* PlayerCharacter);

	// 대기 중인 정보를 새 PlayerCharacter에 한 번 복원한다. 성공하면 대기 상태를 소비한다.
	UFUNCTION(BlueprintCallable, Category = "Player Travel")
	bool RestorePlayerState(APlayerCharacter* PlayerCharacter);

	UFUNCTION(BlueprintPure, Category = "Player Travel")
	bool HasPendingPlayerState() const { return bHasPendingPlayerState; }

private:
	UPROPERTY(Transient)
	bool bHasPendingPlayerState = false;

	UPROPERTY(Transient)
	FTinoPlayerTravelState PendingPlayerState;
};
