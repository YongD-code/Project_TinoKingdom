// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoGameInstance.h"

#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Component/CookingRecipeBookComponent.h"
#include "Project_TinoKingdom/Component/PlayerProgressionComponent.h"
#include "Project_TinoKingdom/Component/QuestComponent.h"
#include "Project_TinoKingdom/Component/TinoEquipmentComponent.h"
#include "Project_TinoKingdom/DataAsset/EquipmentLoadoutData.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAttributeSet.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoGameInstance, Log, All);

bool UTinoGameInstance::CapturePlayerState(APlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	const UPlayerProgressionComponent* ProgressionComponent = PlayerCharacter->GetProgressionComponent();
	const UTinoAttributeSet* AttributeSet = PlayerCharacter->GetAttributeSet();
	const UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent();
	const UCookingRecipeBookComponent* CookingRecipeBookComponent = PlayerCharacter->GetCookingRecipeBookComponent();
	const UQuestComponent* QuestComponent = PlayerCharacter->GetQuestComponent();
	const UTinoEquipmentComponent* EquipmentComponent = PlayerCharacter->GetEquipmentComponent();

	if (!IsValid(ProgressionComponent) || !IsValid(AttributeSet) || !IsValid(InventoryComponent)
		|| !IsValid(CookingRecipeBookComponent) || !IsValid(QuestComponent) || !IsValid(EquipmentComponent))
	{
		UE_LOG(LogTinoGameInstance, Error,
			TEXT("플레이어 이동 상태 저장 실패: 필요한 플레이어 컴포넌트가 없습니다."));
		return false;
	}

	FTinoPlayerTravelState NewState;
	NewState.Level = ProgressionComponent->GetCurrentLevel();
	NewState.Experience = ProgressionComponent->GetCurrentExperience();
	NewState.UnspentStatPoints = ProgressionComponent->GetUnspentStatPoints();

	NewState.Health = AttributeSet->GetHealth();
	NewState.MaxHealth = AttributeSet->GetMaxHealth();
	NewState.Stamina = AttributeSet->GetStamina();
	NewState.MaxStamina = AttributeSet->GetMaxStamina();
	NewState.AttackPower = AttributeSet->GetAttackPower();
	NewState.Defense = AttributeSet->GetDefense();

	NewState.InventoryItems = InventoryComponent->GetItems();
	NewState.DiscoveredCookingRecipes = CookingRecipeBookComponent->GetDiscoveredRecipes();
	NewState.QuestStates = QuestComponent->GetQuestStatesForTravel();
	NewState.TrackedQuest = QuestComponent->GetTrackedQuest();
	NewState.EquipmentLoadout = EquipmentComponent->GetCurrentLoadout();

	PendingPlayerState = MoveTemp(NewState);
	bHasPendingPlayerState = true;

	UE_LOG(LogTinoGameInstance, Log,
		TEXT("플레이어 이동 상태 저장: Level %d, Exp %d, StatPoints %d, Items %d, Quests %d"),
		PendingPlayerState.Level,
		PendingPlayerState.Experience,
		PendingPlayerState.UnspentStatPoints,
		PendingPlayerState.InventoryItems.Num(),
		PendingPlayerState.QuestStates.Num());

	return true;
}

bool UTinoGameInstance::RestorePlayerState(APlayerCharacter* PlayerCharacter)
{
	if (!bHasPendingPlayerState || !IsValid(PlayerCharacter))
	{
		return false;
	}

	UPlayerProgressionComponent* ProgressionComponent = PlayerCharacter->GetProgressionComponent();
	UTinoAttributeSet* AttributeSet = PlayerCharacter->GetMutableAttributeSet();
	UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent();
	UCookingRecipeBookComponent* CookingRecipeBookComponent = PlayerCharacter->GetCookingRecipeBookComponent();
	UQuestComponent* QuestComponent = PlayerCharacter->GetQuestComponent();
	UTinoEquipmentComponent* EquipmentComponent = PlayerCharacter->GetEquipmentComponent();

	if (!IsValid(ProgressionComponent) || !IsValid(AttributeSet) || !IsValid(InventoryComponent)
		|| !IsValid(CookingRecipeBookComponent) || !IsValid(QuestComponent) || !IsValid(EquipmentComponent))
	{
		UE_LOG(LogTinoGameInstance, Error,
			TEXT("플레이어 이동 상태 복원 실패: 필요한 플레이어 컴포넌트가 없습니다."));
		return false;
	}

	ProgressionComponent->RestoreTravelState(
		PendingPlayerState.Level,
		PendingPlayerState.Experience,
		PendingPlayerState.UnspentStatPoints);

	// Max 값을 먼저 복원해야 Current 값이 이전 기본 Max 값에 의해 Clamp되지 않는다.
	AttributeSet->SetMaxHealth(PendingPlayerState.MaxHealth);
	AttributeSet->SetMaxStamina(PendingPlayerState.MaxStamina);
	AttributeSet->SetAttackPower(PendingPlayerState.AttackPower);
	AttributeSet->SetDefense(PendingPlayerState.Defense);
	AttributeSet->SetHealth(PendingPlayerState.Health);
	AttributeSet->SetStamina(PendingPlayerState.Stamina);

	// 퀘스트 진행도는 인벤토리 개수를 참조하므로 인벤토리를 먼저 복원한다.
	InventoryComponent->RestoreItemsForTravel(PendingPlayerState.InventoryItems);
	CookingRecipeBookComponent->RestoreRecipesForTravel(PendingPlayerState.DiscoveredCookingRecipes);
	QuestComponent->RestoreStateForTravel(
		PendingPlayerState.QuestStates,
		PendingPlayerState.TrackedQuest.Get());

	if (IsValid(PendingPlayerState.EquipmentLoadout.Get()))
	{
		EquipmentComponent->EquipLoadout(PendingPlayerState.EquipmentLoadout.Get());
	}

	UE_LOG(LogTinoGameInstance, Log,
		TEXT("플레이어 이동 상태 복원: Level %d, Health %.1f / %.1f, Stamina %.1f / %.1f"),
		PendingPlayerState.Level,
		AttributeSet->GetHealth(),
		AttributeSet->GetMaxHealth(),
		AttributeSet->GetStamina(),
		AttributeSet->GetMaxStamina());

	// 이 데이터는 OpenLevel 한 번을 위한 전달용이다. 이후 Respawn 등에 잘못 재사용하지 않는다.
	bHasPendingPlayerState = false;
	PendingPlayerState = FTinoPlayerTravelState();

	return true;
}
