// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UTinoAnimInstance::UTinoAnimInstance()
{
	MovingSpeedThreshold = 3.f;
	JumpingSpeedThreshold = 0.f;
}

void UTinoAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	OwnerCharacter = Cast<ACharacter>(GetOwningActor());
	if (IsValid(OwnerCharacter))
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}
}

void UTinoAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (IsValid(MovementComponent))
	{
		Velocity = MovementComponent->Velocity;
		GroundSpeed = Velocity.Size2D();
		bIsIdle = GroundSpeed < MovingSpeedThreshold;
		
		const bool bIsInAir = MovementComponent->IsFalling();
		bIsJumping = bIsInAir && (Velocity.Z > JumpingSpeedThreshold);
		bIsFalling = bIsInAir && !bIsJumping;
	}
}
