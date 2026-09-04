// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Project_TinoKingdom/Types/CookingTypes.h"
#include "InventoryComponent.generated.h"


class UTexture2D;
class UStatComponent;

UENUM(BlueprintType)
enum class EInventoryItemType : uint8
{
	Etc UMETA(DisplayName = "Etc"),
	Material UMETA(DisplayName = "Material"),
	Food UMETA(DisplayName = "Food"),
	Equipment UMETA(DisplayName = "Equipment")
};


USTRUCT(BlueprintType)
struct FInventoryItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Count = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UTexture2D> Icon = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	EInventoryItemType ItemType = EInventoryItemType::Etc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking")
	ECookingTag CookingTag = ECookingTag::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking")
	EFoodEffectType FoodEffectType = EFoodEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking", meta = (ClampMin = "0.0"))
	float CookingPower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking")
	FCookingResultData FoodResultData;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnInventoryItemAdded,
	const FInventoryItemStack&,
	ItemStack,
	int32,
	AddedCount
);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(
		FName ItemId,
		FText DisplayName,
		int32 Count,
		UTexture2D* Icon,
		EInventoryItemType ItemType = EInventoryItemType::Etc,
		ECookingTag CookingTag = ECookingTag::None,
		EFoodEffectType FoodEffectType = EFoodEffectType::None,
		float CookingPower = 1.0f,
		const FCookingResultData& FoodResultData = FCookingResultData()
	);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(FName ItemId, int32 Count = 1) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemId, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Food")
	bool UseFoodItem(FName ItemId, UStatComponent* TargetStatComponent);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryItemStack>& GetItems() const { return Items; }

	// 맵 이동 직전 저장한 인벤토리 스택으로 교체한다.
	void RestoreItemsForTravel(const TArray<FInventoryItemStack>& SavedItems);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemAdded OnItemAdded;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TArray<FInventoryItemStack> Items;
};
