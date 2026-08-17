// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/TinoNPCCharacter.h"
#include "BlacksmithNPC.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API ABlacksmithNPC : public ATinoNPCCharacter
{
	GENERATED_BODY()
	
public:
	virtual void StartDialogue() override;
	
};
