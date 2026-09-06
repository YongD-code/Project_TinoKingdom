// Fill out your copyright notice in the Description page of Project Settings.


#include "KingAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"

void UKingAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CacheOwnerReferences();
}

void UKingAnimInstance::NativeUninitializeAnimation()
{
	if (AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(OwnerCharacter))
	{
		EnemyCharacter->UnregisterCombatAnimationMesh(GetSkelMeshComponent());
	}

	MovementComponent = nullptr;
	OwnerCharacter = nullptr;
	ResetLocomotionState();

	Super::NativeUninitializeAnimation();
}

void UKingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(OwnerCharacter) || !IsValid(MovementComponent))
	{
		CacheOwnerReferences();
	}

	if (!IsValid(OwnerCharacter) || !IsValid(MovementComponent))
	{
		ResetLocomotionState();
		return;
	}

	Velocity = MovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();
	bIsMoving = GroundSpeed >= MovingSpeedThreshold;
	bIsIdle = !bIsMoving;
	bIsFalling = MovementComponent->IsFalling();

	if (!bIsMoving)
	{
		MovementDirection = 0.0f;
		return;
	}

	FVector HorizontalVelocity = Velocity;
	HorizontalVelocity.Z = 0.0f;

	const FVector LocalVelocity = OwnerCharacter->GetActorTransform()
		.InverseTransformVectorNoScale(HorizontalVelocity);
	MovementDirection = FMath::RadiansToDegrees(
		FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
}

void UKingAnimInstance::CacheOwnerReferences()
{
	OwnerCharacter = Cast<ACharacter>(GetOwningActor());
	MovementComponent = IsValid(OwnerCharacter)
		? OwnerCharacter->GetCharacterMovement()
		: nullptr;

	if (AEnemyCharacter* EnemyCharacter = Cast<AEnemyCharacter>(OwnerCharacter))
	{
		EnemyCharacter->RegisterCombatAnimationMesh(GetSkelMeshComponent());
	}
}

void UKingAnimInstance::ResetLocomotionState()
{
	Velocity = FVector::ZeroVector;
	GroundSpeed = 0.0f;
	MovementDirection = 0.0f;
	bIsIdle = true;
	bIsMoving = false;
	bIsFalling = false;
}
