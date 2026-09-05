// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

#include "Project_TinoKingdom/Component/StatComponent.h"

namespace
{
// BP_Monkey의 DropItemId와 비밀 장소 입구가 같은 키 스택을 참조하도록 ID를 하나로 통일한다.
const FName MonkeyKeyItemId(TEXT("Monkey"));
const FText MonkeyKeyDisplayName = FText::FromString(TEXT("원숭이 열쇠"));
}

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UInventoryComponent::AddItem(
	FName ItemId,
	FText DisplayName,
	int32 Count,
	UTexture2D* Icon,
	EInventoryItemType ItemType,
	ECookingTag CookingTag,
	EFoodEffectType FoodEffectType,
	float CookingPower,
	const FCookingResultData& FoodResultData
)
{
	if (ItemId.IsNone() || Count <= 0)
	{
		return;
	}

	for (FInventoryItemStack& ItemStack : Items)
	{
		if (ItemStack.ItemId == ItemId)
		{
			ItemStack.Count += Count;
			
			if (ItemStack.Icon == nullptr && Icon != nullptr)
			{
				ItemStack.Icon = Icon;
			}
			
			ItemStack.ItemType = ItemType;
			ItemStack.CookingTag = CookingTag;
			ItemStack.FoodEffectType = FoodEffectType;
			ItemStack.CookingPower = CookingPower;
			ItemStack.FoodResultData = FoodResultData;
			
			OnItemAdded.Broadcast(ItemStack, Count);
			return;
		}
	}

	FInventoryItemStack NewStack;
	NewStack.ItemId = ItemId;
	NewStack.DisplayName = DisplayName;
	NewStack.Count = Count;
	NewStack.Icon = Icon;
	NewStack.ItemType = ItemType;
	NewStack.CookingTag = CookingTag;
	NewStack.FoodEffectType = FoodEffectType;
	NewStack.CookingPower = CookingPower;
	NewStack.FoodResultData = FoodResultData;
	
	Items.Add(NewStack);
	OnItemAdded.Broadcast(NewStack, Count);
		
	//임시로 인벤토리로 들어오는지 확인용 코드
	UE_LOG(LogTemp,Log,TEXT("AddItem: %s x%d / Total: %d"),*DisplayName.ToString(),Count,GetItemCount(ItemId));
}

int32 UInventoryComponent::GetItemCount(FName ItemId) const
{
	for (const FInventoryItemStack& ItemStack : Items)
	{
		if (ItemStack.ItemId == ItemId)
		{
			return ItemStack.Count;
		}
	}

	return 0;
}

bool UInventoryComponent::HasItem(FName ItemId, int32 Count) const
{
	if (ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	return GetItemCount(ItemId) >= Count;
}

bool UInventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	if (ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		FInventoryItemStack& ItemStack = Items[Index];
		if (ItemStack.ItemId != ItemId)
		{
			continue;
		}

		if (ItemStack.Count < Count)
		{
			return false;
		}

		ItemStack.Count -= Count;
		if (ItemStack.Count <= 0)
		{
			Items.RemoveAt(Index);
		}

		return true;
	}

	return false;
}

bool UInventoryComponent::UseFoodItem(FName ItemId, UStatComponent* TargetStatComponent)
{
	if (TargetStatComponent == nullptr || ItemId.IsNone())
	{
		return false;
	}

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		FInventoryItemStack& ItemStack = Items[Index];
		if (ItemStack.ItemId != ItemId)
		{
			continue;
		}

		if (ItemStack.ItemType != EInventoryItemType::Food || ItemStack.Count <= 0)
		{
			return false;
		}

		const FCookingResultData& FoodData = ItemStack.FoodResultData;

		if (!FMath::IsNearlyZero(FoodData.HealAmount))
		{
			TargetStatComponent->Heal(FoodData.HealAmount);
		}

		if (FoodData.StaminaAmount > 0.0f)
		{
			TargetStatComponent->RecoverStamina(FoodData.StaminaAmount);
		}

		ItemStack.Count -= 1;
		if (ItemStack.Count <= 0)
		{
			Items.RemoveAt(Index);
		}

		return true;
	}

	return false;
}

void UInventoryComponent::AddMonkeyKey(UTexture2D* Icon)
{
	AddItem(
		MonkeyKeyItemId,
		MonkeyKeyDisplayName,
		1,
		Icon,
		EInventoryItemType::Key
	);
}

bool UInventoryComponent::HasMonkeyKey() const
{
	return HasItem(MonkeyKeyItemId);
}

FName UInventoryComponent::GetMonkeyKeyItemId()
{
	return MonkeyKeyItemId;
}

void UInventoryComponent::RestoreItemsForTravel(const TArray<FInventoryItemStack>& SavedItems)
{
	Items = SavedItems;
	Items.RemoveAll([](const FInventoryItemStack& ItemStack)
	{
		return ItemStack.ItemId.IsNone() || ItemStack.Count <= 0;
	});
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

