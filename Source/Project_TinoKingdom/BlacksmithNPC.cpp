// Fill out your copyright notice in the Description page of Project Settings.


#include "BlacksmithNPC.h"

void ABlacksmithNPC::StartDialogue(APlayerController* PlayerController)
{
	Super::StartDialogue(PlayerController);
		
	UE_LOG(LogTemp, Warning, TEXT("대장장이 NPC 대화 시작"));
}

TArray<FText> ABlacksmithNPC::GetDialogueLines() const
{
	return {
		FText::FromString(TEXT("요즘 성 밖에 몬스터가 늘어서 여행자가 줄었구만")),
		FText::FromString(TEXT("혹시 괜찮다면 물짱이의 사체 5개를 구해다 줄 수 있겠나?"))
	};
}
