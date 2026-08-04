// Fill out your copyright notice in the Description page of Project Settings.


#include "ReactionComponent.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	bIsReacting = true;
	
	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &UReactionComponent::OnHitMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, DynamicHitMontage);
	
	return true;
}

void UReactionComponent::SetReactionSet(const FEquipmentReactionSet& ReactionSet)
{
	CurrentReactionSet = ReactionSet;
}

// Called when the game starts
void UReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	AnimationMesh = OwnerCharacter->GetMesh();
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

void UReactionComponent::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveHitMontage)
	{
		return;
	}
	ActiveHitMontage = nullptr;
	bIsReacting = false;
}
