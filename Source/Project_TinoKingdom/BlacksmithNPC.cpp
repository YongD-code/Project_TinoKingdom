// Fill out your copyright notice in the Description page of Project Settings.


#include "BlacksmithNPC.h"

void ABlacksmithNPC::StartDialogue()
{
	Super::StartDialogue();
	
	UE_LOG(LogTemp, Warning, TEXT("대장장이 NPC 대화 시작"));
}
