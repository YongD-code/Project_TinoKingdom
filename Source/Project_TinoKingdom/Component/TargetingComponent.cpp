// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetingComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	CameraBoom = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
}
