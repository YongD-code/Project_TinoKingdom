// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TinoEquipmentActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoEquipmentActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATinoEquipmentActor();
	
	void GetWeaponTracePoints(FVector& OutBaseLocation, FVector& OutTipLocation) const;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> EquipmentMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    	TObjectPtr<USceneComponent> AttachmentRoot;

};
