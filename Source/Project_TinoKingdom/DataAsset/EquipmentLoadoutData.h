// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EquipmentLoadoutData.generated.h"

/**
 * 
 */

class ATinoEquipmentActor;
class UAttackComboData;
UCLASS(BlueprintType)
class PROJECT_TINOKINGDOM_API UEquipmentLoadoutData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<ATinoEquipmentActor> RightHandEquipmentClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<ATinoEquipmentActor> LeftHandEquipmentClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAttackComboData> AttackData;
};
