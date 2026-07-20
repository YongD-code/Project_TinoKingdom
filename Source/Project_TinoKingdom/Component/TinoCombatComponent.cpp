// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoCombatComponent.h"

// Sets default values for this component's properties
UTinoCombatComponent::UTinoCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UTinoCombatComponent::InitializeCombat(USkeletalMeshComponent* InAnimationMesh)
{
}


// Called when the game starts
void UTinoCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}
