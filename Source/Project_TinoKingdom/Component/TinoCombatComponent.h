// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TinoCombatComponent.generated.h"


UCLASS( ClassGroup = (Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UTinoCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTinoCombatComponent();
	
	void InitializeCombat(USkeletalMeshComponent* InAnimationMesh);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	
		
};
