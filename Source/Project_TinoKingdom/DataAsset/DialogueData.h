// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueData.generated.h"

class ULevelSequence;

// 대사를 말하는 주체. UI가 이름표와 말풍선 위치를 정하는 데 사용한다.
UENUM(BlueprintType)
enum class EDialogueSpeaker : uint8
{
	NPC,
	Player
};

USTRUCT(BlueprintType)
struct FDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	EDialogueSpeaker Speaker = EDialogueSpeaker::NPC;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (MultiLine = "true"))
	FText Text;

	// 이 대사를 넘긴 직후 재생할 시네마틱.
	// 비어 있으면 곧바로 다음 대사로 넘어간다.
	// 모든 시퀀스를 한꺼번에 메모리에 올리지 않도록 Soft 참조로 보관한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TSoftObjectPtr<ULevelSequence> CinematicAfterLine;
};

UCLASS(BlueprintType)
class PROJECT_TINOKINGDOM_API UDialogueData : public UDataAsset
{
	GENERATED_BODY()

public:
	// UI 이름표에 표시할 화자 이름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText SpeakerDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FDialogueLine> Lines;
};
