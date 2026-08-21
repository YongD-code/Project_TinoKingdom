// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "TinoPlayerController.generated.h"

/**
 * 
 */

class UInputMappingContext;
class UUserWidget;
class UTinoPlayerWidget;

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "UI")
	UTinoPlayerWidget* GetPlayerUIWidget() const { return PlayerUIWidget; }
	
	void SetCrosshairVisible(bool bVisible);
	void SetLockOnMarkerTarget(AActor* NewTarget);
	
protected:
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UTinoPlayerWidget> PlayerUIClass;
	
	UPROPERTY()
	TObjectPtr<UTinoPlayerWidget> PlayerUIWidget;
};
