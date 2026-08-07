// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoEquipmentActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

namespace
{
	const FName WeaponTraceBaseSocketName(TEXT("WeaponTraceBase"));
	const FName WeaponTraceTipSocketName(TEXT("WeaponTraceTip"));
}

// Sets default values
ATinoEquipmentActor::ATinoEquipmentActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AttachmentRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AttachmentRoot"));
	SetRootComponent(AttachmentRoot);
	
	EquipmentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquipmentMesh"));
	EquipmentMesh->SetupAttachment(AttachmentRoot);
	
	EquipmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquipmentMesh->SetGenerateOverlapEvents(false);
	EquipmentMesh->SetCanEverAffectNavigation(false);
}

void ATinoEquipmentActor::GetWeaponTracePoints(FVector& OutBaseLocation, FVector& OutTipLocation) const
{
	checkf(EquipmentMesh->DoesSocketExist(WeaponTraceBaseSocketName), TEXT("%s : 무기에 %s이 없습니다."),
		*GetNameSafe(this), *WeaponTraceBaseSocketName.ToString());
	checkf(EquipmentMesh->DoesSocketExist(WeaponTraceTipSocketName), TEXT("%s : 무기에 %s이 없습니다."),
		*GetNameSafe(this), *WeaponTraceTipSocketName.ToString());
	
	OutBaseLocation = EquipmentMesh->GetSocketLocation(WeaponTraceBaseSocketName);
	OutTipLocation = EquipmentMesh->GetSocketLocation(WeaponTraceTipSocketName);
}
