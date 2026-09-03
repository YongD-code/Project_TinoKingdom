// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Project_TinoKingdom/Types/CookingTypes.h"
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

UENUM(BlueprintType)
enum class EQuestObjectiveType : uint8
{
	Item UMETA(DisplayName = "Item"),
	Cooking UMETA(DisplayName = "Cooking")
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

	// 일반 아이템과 요리 중 어떤 종류를 목표로 삼을지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	EQuestObjectiveType ObjectiveType = EQuestObjectiveType::Item;

	// 모아야 할 일반 아이템. EnemyCharacter의 DropItemId와 같은 값을 넣는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective",
		meta = (EditCondition = "ObjectiveType == EQuestObjectiveType::Item", EditConditionHides))
	FName TargetItemId = NAME_None;

	// 요리 목표일 때 요구하는 주재료, 결과 종류, 품질이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective|Cooking",
		meta = (EditCondition = "ObjectiveType == EQuestObjectiveType::Cooking", EditConditionHides))
	ECookingTag RequiredCookingMainTag = ECookingTag::Fish;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective|Cooking",
		meta = (EditCondition = "ObjectiveType == EQuestObjectiveType::Cooking", EditConditionHides))
	ECookingResultType RequiredCookingResultType = ECookingResultType::Soup;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective|Cooking",
		meta = (EditCondition = "ObjectiveType == EQuestObjectiveType::Cooking", EditConditionHides))
	ECookingQuality RequiredCookingQuality = ECookingQuality::Special;

	// 목표 UI에 표시할 아이템 이름. 예) 물짱이의 사체
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	FText TargetItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective", meta = (ClampMin = "1"))
	int32 RequiredCount = 2;
};
