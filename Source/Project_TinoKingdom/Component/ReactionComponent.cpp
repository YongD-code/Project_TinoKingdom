// Fill out your copyright notice in the Description page of Project Settings.


#include "ReactionComponent.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Project_TinoKingdom/Component/TinoStateComponent.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"

// Sets default values for this component's properties
UReactionComponent::UReactionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

bool UReactionComponent::PlayHitReaction(AActor* DamageCauser)
{
	const EReactionDirection HitDirection = CalculateHitDirection(DamageCauser);
	UAnimSequenceBase* HitAnimation = GetHitAnimation(HitDirection);
	if (HitAnimation == nullptr)
	{
		return false;
	}
	
	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (MovementComponent->IsMovingOnGround())
	{
		OwnerCharacter->ConsumeMovementInputVector();
		MovementComponent->StopMovementImmediately();
	}
	
	UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance();
	UAnimMontage* DynamicHitMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
		HitAnimation, HitReactionSlotName,
		CurrentReactionSet.HitBlendInTime, CurrentReactionSet.HitBlendOutTime,
		1.f, 1);
	
	ActiveHitMontage = DynamicHitMontage;
	SetReacting(true);
	
	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &UReactionComponent::OnHitMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, DynamicHitMontage);
	
	return true;
}

bool UReactionComponent::PlayDeathReaction(AActor* DamageCauser)
{
	if (bDeathReactionPlayed)
	{
		return false;
	}
	const EReactionDirection DeathDirection = CalculateHitDirection(DamageCauser);
	const FName DeathSectionName = GetDeathSectionName(DeathDirection);
	UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance();
	
	if (AnimInstance->Montage_Play(DeathMontage) <= 0.f)
	{
		return false;
	}
	AnimInstance->Montage_JumpToSection(DeathSectionName, DeathMontage);
	
	bDeathReactionPlayed = true;
	ActiveHitMontage = nullptr;
	SetReacting(false);
	
	return true;
}

void UReactionComponent::SetReactionSet(const FEquipmentReactionSet& ReactionSet)
{
	CurrentReactionSet = ReactionSet;
}

void UReactionComponent::ResetDeathReaction()
{
	if (IsValid(AnimationMesh))
	{
		if (UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance())
		{
			if (DeathMontage != nullptr)
			{
				AnimInstance->Montage_Stop(0.f, DeathMontage);
			}
		}
	}
	
	bDeathReactionPlayed = false;
	ActiveHitMontage = nullptr;
	SetReacting(false);
}

// Called when the game starts
void UReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	AnimationMesh = OwnerCharacter->GetMesh();
	StateComponent = OwnerCharacter->FindComponentByClass<UTinoStateComponent>();
}

EReactionDirection UReactionComponent::CalculateHitDirection(const AActor* DamageCauser)
{
	if (DamageCauser == nullptr)
	{
		return EReactionDirection::Front;
	}
	
	const FVector PlayerLookEnemyDir = DamageCauser->GetActorLocation() - OwnerCharacter->GetActorLocation();
	const float ForwardDot = PlayerLookEnemyDir.Dot(OwnerCharacter->GetActorForwardVector());
	const float RightDot = PlayerLookEnemyDir.Dot(OwnerCharacter->GetActorRightVector());
	
	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		return ForwardDot >= 0.f ? EReactionDirection::Front : EReactionDirection::Back;
	}
	return RightDot >= 0.f ? EReactionDirection::Right : EReactionDirection::Left;
}

UAnimSequenceBase* UReactionComponent::GetHitAnimation(EReactionDirection HitDirection) const
{
	const FDirectionalHitAnimations& Animations = CurrentReactionSet.HitAnimations;
	switch (HitDirection)
	{
		case EReactionDirection::Front: return Animations.Front;
		case EReactionDirection::Back: return Animations.Back;
		case EReactionDirection::Left: return Animations.Left;
		case EReactionDirection::Right: return Animations.Right;
	}
	return Animations.Front;
}

FName UReactionComponent::GetDeathSectionName(EReactionDirection ReactionDirection) const
{
	switch (ReactionDirection)
	{
		case EReactionDirection::Front: return FName("FromFront");
		case EReactionDirection::Back: return FName("FromBack");
		case EReactionDirection::Left: return FName("FromLeft");
		case EReactionDirection::Right: return FName("FromRight");
	}
	return FName("FromFront");
}

void UReactionComponent::SetReacting(bool bNewIsReacting)
{
	if (bIsReacting == bNewIsReacting)
	{
		return;
	}
	bIsReacting = bNewIsReacting;
	if (bIsReacting)
	{
		StateComponent->AddStateTag(TinoGameplayTags::State_Action_HitReacting);
	}
	else
	{
		StateComponent->RemoveStateTag(TinoGameplayTags::State_Action_HitReacting);
	}
}

void UReactionComponent::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveHitMontage)
	{
		return;
	}
	ActiveHitMontage = nullptr;
	SetReacting(false);
}
