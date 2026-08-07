#pragma once

#include "CoreMinimal.h"
#include "ReactionTypes.generated.h"

class UAnimSequenceBase;

USTRUCT(BlueprintType)
struct PROJECT_TINOKINGDOM_API FDirectionalHitAnimations
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<UAnimSequenceBase> Front;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<UAnimSequenceBase> Back;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<UAnimSequenceBase> Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	TObjectPtr<UAnimSequenceBase> Right;
};

USTRUCT(BlueprintType)
struct PROJECT_TINOKINGDOM_API FEquipmentReactionSet
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	FDirectionalHitAnimations HitAnimations;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float HitBlendInTime = 0.05f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float HitBlendOutTime = 0.15f;
};