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

void UInventoryComponent::AddItem(FName ItemId, FText DisplayName, int32 Count)
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
			OnItemAdded.Broadcast(ItemStack, Count);
			return;
		}
	}

	FInventoryItemStack NewStack;
	NewStack.ItemId = ItemId;
	NewStack.DisplayName = DisplayName;
	NewStack.Count = Count;

	Items.Add(NewStack);
	OnItemAdded.Broadcast(NewStack, Count);
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

