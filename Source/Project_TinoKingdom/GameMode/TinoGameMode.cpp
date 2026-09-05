// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoGameMode.h"

#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Player/TinoPlayerController.h"
#include "Kismet/GameplayStatics.h"

ATinoGameMode::ATinoGameMode()
{
	PlayerControllerClass = ATinoPlayerController::StaticClass();
	DefaultPawnClass = APlayerCharacter::StaticClass();
}

void ATinoGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(BackgroundMusic))
	{
		UGameplayStatics::PlaySound2D(this, BackgroundMusic);
	}
}
