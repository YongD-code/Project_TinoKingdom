// Fill out your copyright notice in the Description page of Project Settings.


#include "QuestComponent.h"

#include "Project_TinoKingdom/Component/InventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoQuest, Log, All);

UQuestComponent::UQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		InventoryComponent = Owner->FindComponentByClass<UInventoryComponent>();
	}

	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnItemAdded.AddUniqueDynamic(this, &UQuestComponent::HandleItemAdded);
	}
}

void UQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnItemAdded.RemoveDynamic(this, &UQuestComponent::HandleItemAdded);
	}

	Super::EndPlay(EndPlayReason);
}

EQuestState UQuestComponent::GetQuestState(const UQuestData* Quest) const
{
	if (!IsValid(Quest))
	{
		return EQuestState::NotStarted;
	}

	const EQuestState* Found = QuestStates.Find(Quest);

	return (Found != nullptr) ? *Found : EQuestState::NotStarted;
}

int32 UQuestComponent::GetQuestProgress(const UQuestData* Quest) const
{
	if (!IsValid(Quest) || !IsValid(InventoryComponent))
	{
		return 0;
	}

	// 별도 카운터를 두지 않고 인벤토리를 직접 센다.
	// 아이템을 버리거나 세이브를 불러와도 표시가 어긋나지 않는다.
	const int32 Count = InventoryComponent->GetItemCount(Quest->TargetItemId);

	return FMath::Min(Count, Quest->RequiredCount);
}

bool UQuestComponent::AcceptQuest(UQuestData* Quest)
{
	if (!IsValid(Quest))
	{
		return false;
	}

	if (GetQuestState(Quest) != EQuestState::NotStarted)
	{
		return false;
	}

	QuestStates.Add(Quest, EQuestState::InProgress);
	TrackedQuest = Quest;

	OnQuestAccepted.Broadcast(Quest);

	UE_LOG(LogTinoQuest, Log, TEXT("퀘스트 수령: %s"), *Quest->Title.ToString());

	// 수령 시점에 이미 아이템을 갖고 있을 수 있으므로 바로 한 번 갱신한다.
	RefreshProgress(Quest);

	return true;
}

bool UQuestComponent::CompleteQuest(UQuestData* Quest)
{
	if (!IsValid(Quest) || GetQuestState(Quest) != EQuestState::ReadyToComplete)
	{
		return false;
	}

	QuestStates.Add(Quest, EQuestState::Completed);

	if (TrackedQuest == Quest)
	{
		TrackedQuest = nullptr;
	}

	OnQuestCompleted.Broadcast(Quest);

	UE_LOG(LogTinoQuest, Log, TEXT("퀘스트 완료: %s"), *Quest->Title.ToString());

	return true;
}

void UQuestComponent::RestoreStateForTravel(
	const TMap<TObjectPtr<UQuestData>, EQuestState>& SavedQuestStates,
	UQuestData* SavedTrackedQuest)
{
	QuestStates.Reset();
	TrackedQuest = nullptr;

	for (const TPair<TObjectPtr<UQuestData>, EQuestState>& Pair : SavedQuestStates)
	{
		if (!IsValid(Pair.Key.Get()) || Pair.Value == EQuestState::NotStarted)
		{
			continue;
		}

		QuestStates.Add(Pair.Key, Pair.Value);
	}

	if (IsValid(SavedTrackedQuest))
	{
		const EQuestState RestoredState = GetQuestState(SavedTrackedQuest);
		if (RestoredState == EQuestState::InProgress || RestoredState == EQuestState::ReadyToComplete)
		{
			TrackedQuest = SavedTrackedQuest;
		}
	}

	// 인벤토리 복원 결과와 퀘스트 상태가 어긋나지 않도록 진행 중인 목표를 다시 계산한다.
	if (IsValid(TrackedQuest) && GetQuestState(TrackedQuest) == EQuestState::InProgress)
	{
		RefreshProgress(TrackedQuest);
	}
}

void UQuestComponent::HandleItemAdded(const FInventoryItemStack& ItemStack, int32 AddedCount)
{
	if (!IsValid(TrackedQuest))
	{
		return;
	}

	// 추적 중인 퀘스트와 무관한 아이템이면 계산할 필요가 없다.
	if (ItemStack.ItemId != TrackedQuest->TargetItemId)
	{
		return;
	}

	RefreshProgress(TrackedQuest);
}

void UQuestComponent::RefreshProgress(UQuestData* Quest)
{
	if (!IsValid(Quest) || GetQuestState(Quest) != EQuestState::InProgress)
	{
		return;
	}

	const int32 Current = GetQuestProgress(Quest);

	// UI를 붙이기 전에도 진행도를 확인할 수 있도록 남긴다.
	UE_LOG(LogTinoQuest, Log, TEXT("퀘스트 진행: %s  %d / %d"),
		*Quest->Title.ToString(), Current, Quest->RequiredCount);

	OnQuestProgressChanged.Broadcast(Quest, Current, Quest->RequiredCount);

	if (Current < Quest->RequiredCount)
	{
		return;
	}

	QuestStates.Add(Quest, EQuestState::ReadyToComplete);

	OnQuestReadyToComplete.Broadcast(Quest);

	UE_LOG(LogTinoQuest, Log, TEXT("퀘스트 목표 달성: %s"), *Quest->Title.ToString());
}
