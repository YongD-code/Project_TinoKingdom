// Fill out your copyright notice in the Description page of Project Settings.


#include "DodgeComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Project_TinoKingdom/Component/TinoStateComponent.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"

namespace DodgeSections
{
	const FName Forward(TEXT("Forward"));
	const FName Backward(TEXT("Backward"));
	const FName Left(TEXT("Left"));
	const FName Right(TEXT("Right"));
}
// Sets default values for this component's properties
UDodgeComponent::UDodgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDodgeComponent::StartDodge(const FVector& DodgeDirection, bool bUseStrafeDodge)
{
	if (ActiveDodgeMontage != nullptr)
	{
		return;
	}
	
	FVector NormalizedDirection = DodgeDirection.GetSafeNormal2D();
	if (!bUseStrafeDodge)
	{
		OwnerCharacter->SetActorRotation(NormalizedDirection.Rotation());
	}
	OwnerCharacter->ConsumeMovementInputVector();
	OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
	
	ActiveDodgeMontage = bUseStrafeDodge ? StrafeDodgeMontage : DodgeMontage;
	const FName SelectedSection = bUseStrafeDodge ? SelectStrafeDodgeSection(NormalizedDirection) : NAME_None;
	StateComponent->AddStateTag(TinoGameplayTags::State_Action_Dodging);
	
	UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance();
	if (AnimInstance->Montage_Play(ActiveDodgeMontage) <= 0.f)
	{
		FinishDodge();
		return;
	}
	
	if (bUseStrafeDodge)
	{
		AnimInstance->Montage_SetNextSection(SelectedSection, NAME_None, ActiveDodgeMontage);
		AnimInstance->Montage_JumpToSection(SelectedSection, ActiveDodgeMontage);
	}
	
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

void UDodgeComponent::BeginInvincibilityWindow()
{
	if (ActiveDodgeMontage == nullptr)
	{
		return;
	}
	if (bInvincibilityWindowActive)
	{
		return;
	}
	bInvincibilityWindowActive = true;
	StateComponent->AddStateTag(TinoGameplayTags::State_Invincible);
}

void UDodgeComponent::EndInvincibilityWindow()
{
	if (!bInvincibilityWindowActive)
	{
		return;
	}
	bInvincibilityWindowActive = false;
	StateComponent->RemoveStateTag(TinoGameplayTags::State_Invincible);
}


// Called when the game starts
void UDodgeComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	AnimationMesh = OwnerCharacter->GetMesh();
	StateComponent = OwnerCharacter->FindComponentByClass<UTinoStateComponent>();
}

FName UDodgeComponent::SelectStrafeDodgeSection(const FVector& DodgeDirection)
{
	const FVector LocalDirection = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale(DodgeDirection);
	if (FMath::Abs(LocalDirection.X) >= FMath::Abs(LocalDirection.Y))
	{
		return LocalDirection.X >= 0.f ? DodgeSections::Forward : DodgeSections::Backward;
	}
	return LocalDirection.Y >= 0.f ? DodgeSections::Right : DodgeSections::Left;
}

// 정상 종료와 강제적으로 발생한 외부 종료에 공통으로 사용
void UDodgeComponent::FinishDodge()
{
	if (ActiveDodgeMontage == nullptr)
	{
		return;
	}
	EndInvincibilityWindow();
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
