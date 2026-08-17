// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoNPCCharacter.h"

// Sets default values
ATinoNPCCharacter::ATinoNPCCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATinoNPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATinoNPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATinoNPCCharacter::StartDialogue()
{
	UE_LOG(LogTemp, Warning, TEXT("NPC 대화 시작"));
}

void ATinoNPCCharacter::EndDialogue()
{
}

