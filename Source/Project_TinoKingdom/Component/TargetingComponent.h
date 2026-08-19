// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

class USpringArmComponent;
class AActor;
class ACharacter;

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetingComponent();

	UFUNCTION(BlueprintPure, Category = "Targeting")
	bool IsLockedOn() const { return CurrentTarget.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Targeting")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }
	
protected:
	virtual void BeginPlay() override;
	
	
private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTarget;
	
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

};
