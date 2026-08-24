// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

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

