// Fill out your copyright notice in the Description page of Project Settings.


#include "InvincibilityWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Project_TinoKingdom/Component/DodgeComponent.h"

UDodgeComponent* FindDodgeComponent(const USkeletalMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return nullptr;
	}
	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner))
	{
		return nullptr;
	}
	return Owner->FindComponentByClass<UDodgeComponent>();
}

void UInvincibilityWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (UDodgeComponent* DodgeComponent = FindDodgeComponent(MeshComp))
	{
		DodgeComponent->BeginInvincibilityWindow();
	}
}

void UInvincibilityWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (UDodgeComponent* DodgeComponent = FindDodgeComponent(MeshComp))
	{
		DodgeComponent->EndInvincibilityWindow();
	}
}
