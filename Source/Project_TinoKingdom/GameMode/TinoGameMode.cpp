// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoGameMode.h"

#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Player/TinoPlayerController.h"

ATinoGameMode::ATinoGameMode()
{
	PlayerControllerClass = ATinoPlayerController::StaticClass();
	DefaultPawnClass = APlayerCharacter::StaticClass();
}
