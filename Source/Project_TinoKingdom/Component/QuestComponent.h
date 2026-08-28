// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Project_TinoKingdom/DataAsset/QuestData.h"
#include "QuestComponent.generated.h"

class UInventoryComponent;
struct FInventoryItemStack;

// UI가 C++을 직접 참조하지 않고 퀘스트 상태에 반응할 수 있도록 델리게이트로 알린다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestAccepted, UQuestData*, Quest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestReadyToComplete, UQuestData*, Quest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestCompleted, UQuestData*, Quest);

// 진행도가 바뀔 때마다 현재 개수와 목표 개수를 함께 보낸다. 예) 3 / 5
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnQuestProgressChanged, UQuestData*, Quest, int32, Current, int32, Required);

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuestComponent();

	// 퀘스트를 수령한다. 이미 수령했거나 끝난 퀘스트면 아무것도 하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool AcceptQuest(UQuestData* Quest);

	// 목표를 다 채운 퀘스트를 완료 처리한다.
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CompleteQuest(UQuestData* Quest);

	UFUNCTION(BlueprintPure, Category = "Quest")
	EQuestState GetQuestState(const UQuestData* Quest) const;

	// 현재 모은 개수. 인벤토리를 직접 세므로 별도 카운터와 어긋날 일이 없다.
	UFUNCTION(BlueprintPure, Category = "Quest")
	int32 GetQuestProgress(const UQuestData* Quest) const;

	// 추적 UI에 표시할 퀘스트. 진행 중인 것이 없으면 nullptr.
	UFUNCTION(BlueprintPure, Category = "Quest")
	UQuestData* GetTrackedQuest() const { return TrackedQuest; }

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FOnQuestAccepted OnQuestAccepted;

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FOnQuestProgressChanged OnQuestProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FOnQuestReadyToComplete OnQuestReadyToComplete;

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FOnQuestCompleted OnQuestCompleted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 인벤토리가 바뀔 때마다 추적 중인 퀘스트의 진행도를 다시 계산한다.
	UFUNCTION()
	void HandleItemAdded(const FInventoryItemStack& ItemStack, int32 AddedCount);

	// 진행도를 다시 세고, 목표를 채웠으면 완료 가능 상태로 올린다.
	void RefreshProgress(UQuestData* Quest);

private:
	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	// 퀘스트별 상태. 수령한 적 없는 퀘스트는 아예 들어 있지 않다.
	UPROPERTY(Transient)
	TMap<TObjectPtr<UQuestData>, EQuestState> QuestStates;

	UPROPERTY(Transient)
	TObjectPtr<UQuestData> TrackedQuest;
};
