#pragma once

#include "CoreMinimal.h"
#include "CookingTypes.generated.h"

UENUM(BlueprintType)
enum class ECookingTag : uint8
{
	None UMETA(DisplayName = "None"),

	Fish UMETA(DisplayName = "Fish"),
	Slime UMETA(DisplayName = "Slime"),
	Mushroom UMETA(DisplayName = "Mushroom"),
	Meat UMETA(DisplayName = "Meat"),
	Herb UMETA(DisplayName = "Herb"),
	Wood UMETA(DisplayName = "Wood"),
	Monster UMETA(DisplayName = "Monster")
};

UENUM(BlueprintType)
enum class ECookingResultType : uint8
{
	None UMETA(DisplayName = "None"),

	Jelly UMETA(DisplayName = "Jelly"),
	Soup UMETA(DisplayName = "Soup"),
	Grill UMETA(DisplayName = "Grill"),
	Failed UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class ECookingQuality : uint8
{
	Failed UMETA(DisplayName = "Failed"),
	Normal UMETA(DisplayName = "Normal"),
	Good UMETA(DisplayName = "Good"),
	Special UMETA(DisplayName = "Special")
};

UENUM(BlueprintType)
enum class EFoodEffectType : uint8
{
	None UMETA(DisplayName = "None"),

	Heal UMETA(DisplayName = "Heal"),
	Stamina UMETA(DisplayName = "Stamina"),
	AttackBuff UMETA(DisplayName = "Attack Buff"),
	DefenseBuff UMETA(DisplayName = "Defense Buff")
};

USTRUCT(BlueprintType)
struct FCookingIngredientData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	ECookingTag CookingTag = ECookingTag::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	EFoodEffectType EffectType = EFoodEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking", meta = (ClampMin = "0.0"))
	float CookingPower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking", meta = (ClampMin = "1"))
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FCookingIconData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon")
	ECookingResultType BaseType = ECookingResultType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon")
	ECookingTag MainTag = ECookingTag::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon")
	ECookingTag SubTag = ECookingTag::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon")
	FLinearColor BaseTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FishRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlimeRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MushroomRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MeatRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HerbRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WoodRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking|Icon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MonsterRatio = 0.0f;
};

USTRUCT(BlueprintType)
struct FCookingResultData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	FName ResultItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	FText ResultName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	ECookingResultType ResultType = ECookingResultType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	ECookingQuality Quality = ECookingQuality::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	float HealAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	float StaminaAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	float AttackBuffAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	float DefenseBuffAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	FCookingIconData IconData;
};
