// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuestData.generated.h"

UENUM(BlueprintType)
enum class EQuestState : uint8
{
	// 아직 수령하지 않음
	NotStarted,
	// 수령했고 목표를 채우는 중
	InProgress,
	// 목표를 다 채워 NPC에게 보고만 하면 되는 상태
	ReadyToComplete,
	// 보상까지 받고 끝난 상태
	Completed
};

UCLASS(BlueprintType)
class PROJECT_TINOKINGDOM_API UQuestData : public UDataAsset
{
	GENERATED_BODY()

public:
	// 퀘스트를 구분하는 고유 이름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestId = NAME_None;

	// 수락 시 화면에 띄울 제목. 예) 늑대의 가죽을 모으자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (MultiLine = "true"))
	FText Description;

	// 모아야 할 아이템. EnemyCharacter의 DropItemId와 같은 값을 넣는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	FName TargetItemId = NAME_None;

	// 목표 UI에 표시할 아이템 이름. 예) 물짱이의 사체
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	FText TargetItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective", meta = (ClampMin = "1"))
	int32 RequiredCount = 5;
};
