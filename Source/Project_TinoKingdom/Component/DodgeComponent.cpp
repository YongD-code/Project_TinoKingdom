// Fill out your copyright notice in the Description page of Project Settings.


#include "DodgeComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Project_TinoKingdom/Component/TinoStateComponent.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"

// Sets default values for this component's properties
UDodgeComponent::UDodgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDodgeComponent::StartDodge(const FVector& DodgeDirection)
{
	if (ActiveDodgeMontage != nullptr)
	{
		return;
	}
	
	FVector NormalizedDirection = DodgeDirection.GetSafeNormal2D();

	OwnerCharacter->SetActorRotation(NormalizedDirection.Rotation());
	OwnerCharacter->ConsumeMovementInputVector();
	OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
	
	ActiveDodgeMontage = DodgeMontage;
	StateComponent->AddStateTag(TinoGameplayTags::State_Action_Dodging);
	
	UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance();
	AnimInstance->Montage_Play(ActiveDodgeMontage);
	
	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &UDodgeComponent::OnDodgeMontageEnded);
	
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveDodgeMontage);
}

void UDodgeComponent::CancelDodge()
{
	if (ActiveDodgeMontage == nullptr)
	{
		return;
	}
	UAnimMontage* MontageToStop = ActiveDodgeMontage;
	
	FinishDodge();
	
	UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance();
	if (AnimInstance->Montage_IsPlaying(MontageToStop))
	{
		constexpr float BlendOutTime = 0.05f;
		AnimInstance->Montage_Stop(BlendOutTime, MontageToStop);
	}
}


// Called when the game starts
void UDodgeComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	AnimationMesh = OwnerCharacter->GetMesh();
	StateComponent = OwnerCharacter->FindComponentByClass<UTinoStateComponent>();
}

// 정상 종료와 강제적으로 발생한 외부 종료에 공통으로 사용
void UDodgeComponent::FinishDodge()
{
	if (ActiveDodgeMontage == nullptr)
	{
		return;
	}
	ActiveDodgeMontage = nullptr;
	StateComponent->RemoveStateTag(TinoGameplayTags::State_Action_Dodging);
}

void UDodgeComponent::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveDodgeMontage)
	{
		return;
	}
	FinishDodge();
}
