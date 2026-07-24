// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoEquipmentActor.h"

#include "Components/StaticMeshComponent.h"

// Sets default values
ATinoEquipmentActor::ATinoEquipmentActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	EquipmentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquipmentMesh"));
	SetRootComponent(EquipmentMesh);
	
	EquipmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquipmentMesh->SetGenerateOverlapEvents(false);
	EquipmentMesh->SetCanEverAffectNavigation(false);
}
