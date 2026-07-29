// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EquipmentLoadoutData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EEquipmentGrade : uint8
{
	Unrated UMETA(DisplayName = "미설정"),
	Low UMETA(DisplayName = "하"),
	Medium UMETA(DisplayName = "중"),
	High UMETA(DisplayName = "상")
};

class ATinoEquipmentActor;
class UAttackComboData;
class UTexture2D;

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
	
	// UI에 표시할 장비 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText DisplayName;
	
	// UI에 표시할 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UTexture2D> Icon;
	
	// UI에 표시할 피해량 등급
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	EEquipmentGrade DamageGrade = EEquipmentGrade::Unrated;
	
	// UI에 표시할 사거리 등급
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	EEquipmentGrade ReachGrade = EEquipmentGrade::Unrated;
};
