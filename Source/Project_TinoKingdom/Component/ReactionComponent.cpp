// Fill out your copyright notice in the Description page of Project Settings.


#include "ReactionComponent.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TinoCombatComponent.h"

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
	// 피격되면 현재 공격 상태를 종료한다.
	CombatComponent->CancelAttack();
	
	const EHitDirection HitDirection = CalculateHitDirection(DamageCauser);
	UAnimMontage* HitMontage = GetHitMontage(HitDirection);
	if (HitMontage == nullptr)
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
	// HitMontage를 플레이, 다 재생했다면 false로 리턴
	if (AnimInstance->Montage_Play(HitMontage) <= 0.f)
	{
		return false;
	}
	
	ActiveHitMontage = HitMontage;
	bIsReacting = true;
	
	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &UReactionComponent::OnHitMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, HitMontage);
	
	return true;
}

// Called when the game starts
void UReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	AnimationMesh = OwnerCharacter->GetMesh();
	CombatComponent = OwnerCharacter->FindComponentByClass<UTinoCombatComponent>();
}

EHitDirection UReactionComponent::CalculateHitDirection(const AActor* DamageCauser)
{
	if (DamageCauser == nullptr)
	{
		return EHitDirection::Front;
	}
	
	const FVector PlayerLookEnemyDir = DamageCauser->GetActorLocation() - OwnerCharacter->GetActorLocation();
	const float ForwardDot = PlayerLookEnemyDir.Dot(OwnerCharacter->GetActorForwardVector());
	const float RightDot = PlayerLookEnemyDir.Dot(OwnerCharacter->GetActorRightVector());
	
	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		return ForwardDot >= 0.f ? EHitDirection::Front : EHitDirection::Back;
	}
	return RightDot >= 0.f ? EHitDirection::Right : EHitDirection::Left;
}

UAnimMontage* UReactionComponent::GetHitMontage(EHitDirection HitDirection) const
{
	UAnimMontage* HitMontage = nullptr;
	switch (HitDirection)
	{
		case EHitDirection::Front: HitMontage = HitFromFrontMontage; break;
		case EHitDirection::Back: HitMontage = HitFromBackMontage; break;
		case EHitDirection::Left: HitMontage = HitFromLeftMontage; break;
		case EHitDirection::Right: HitMontage = HitFromRightMontage; break;
	}
	return HitMontage;
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
