// Fill out your copyright notice in the Description page of Project Settings.


#include "FootstepAnimNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/GameMode/TinoGameMode.h"
#include "Sound/SoundBase.h"

void UFootstepAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	const ATinoGameMode* GameMode = Cast<ATinoGameMode>(UGameplayStatics::GetGameMode(MeshComp));

	if (!IsValid(GameMode))
	{
		return;
	}

	USoundBase* FootstepSound = GameMode->GetFootstepSound();

	if (!IsValid(FootstepSound))
	{
		return;
	}

	// 소켓이 지정돼 있으면 그 발 위치에서, 아니면 캐릭터 위치에서 낸다.
	const FVector Location = MeshComp->DoesSocketExist(FootSocketName)
		? MeshComp->GetSocketLocation(FootSocketName)
		: MeshComp->GetComponentLocation();

	UGameplayStatics::PlaySoundAtLocation(MeshComp, FootstepSound, Location);
}
