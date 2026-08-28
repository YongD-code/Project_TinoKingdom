// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/TinoNPCCharacter.h"
#include "BlacksmithNPC.generated.h"

/**
 * 대장장이 NPC.
 * 대사는 DialogueData 애셋에서 지정하므로 이 클래스는 대장장이 고유 동작만 담는다.
 */
UCLASS()
class PROJECT_TINOKINGDOM_API ABlacksmithNPC : public ATinoNPCCharacter
{
	GENERATED_BODY()
};
