// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TinoNPCCharacter.generated.h"

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATinoNPCCharacter();

protected:
	// 게임시작 또는 호출되었을때 실행
	virtual void BeginPlay() override;

public:	
	// 매프레임 실행
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	virtual void StartDialogue();
	
	UFUNCTION(BlueprintCallable)
	virtual void EndDialogue();
};
