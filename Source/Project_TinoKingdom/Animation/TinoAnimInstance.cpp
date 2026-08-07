// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoAnimInstance.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Project_TinoKingdom/Component/TinoEquipmentComponent.h"
#include "Project_TinoKingdom/DataAsset/EquipmentLoadoutData.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoAnimation, Log, All);

UTinoAnimInstance::UTinoAnimInstance()
{
	MovingSpeedThreshold = 3.f;
	JumpingSpeedThreshold = 0.f;
}

void UTinoAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	// AnimInstance가 재초기화되는 경우 기존 바인딩부터 제거한다.
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UTinoAnimInstance::HandleEquipmentChanged);
	}
	
	OwnerCharacter = Cast<ACharacter>(GetOwningActor());
	if (!IsValid(OwnerCharacter))
	{
		return;
	}
	MovementComponent = OwnerCharacter->GetCharacterMovement();
	EquipmentComponent = OwnerCharacter->FindComponentByClass<UTinoEquipmentComponent>();
	if (!IsValid(EquipmentComponent))
	{
		UE_LOG(LogTinoAnimation, Warning, TEXT("%s: TinoEquipmentComponent가 없습니다."), *GetNameSafe(OwnerCharacter));
		return;
	}
	EquipmentComponent->OnEquipmentChanged.AddUniqueDynamic(this, &UTinoAnimInstance::HandleEquipmentChanged);
	
	// EquipmentComponent가 먼저 BeginPlay한 경우 초기 Broadcast를 놓쳤을 수 있다.
	if (UEquipmentLoadoutData* CurrentLoadout = EquipmentComponent->GetCurrentLoadout())
	{
		HandleEquipmentChanged(CurrentLoadout);
	}
}

void UTinoAnimInstance::NativeUninitializeAnimation()
{
	if (IsValid(EquipmentComponent))
	{
		EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UTinoAnimInstance::HandleEquipmentChanged);
	}
	EquipmentComponent = nullptr;
	MovementComponent = nullptr;
	OwnerCharacter = nullptr;
	
	Super::NativeUninitializeAnimation();
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
		
		const bool bIsGrounded = !bIsInAir;
		bShouldPlayLandAnimation = bIsGrounded && (GroundSpeed < LandingSpeedThreshold);
		bShouldSkipLandAnimation = bIsGrounded && (GroundSpeed >= LandingSpeedThreshold);
	}
}

void UTinoAnimInstance::HandleEquipmentChanged(UEquipmentLoadoutData* NewLoadout)
{
	if (!IsValid(NewLoadout))
	{
		return;
	}
	
	const FEquipmentLocomotionSet& Locomotion = NewLoadout->Locomotion;
	const bool bHasCompleteLocomotionSet =
		IsValid(Locomotion.GroundMovement.Get()) &&
		IsValid(Locomotion.Jump.Get()) &&
		IsValid(Locomotion.FallLoop.Get()) &&
		IsValid(Locomotion.Land.Get());
	
	if (!bHasCompleteLocomotionSet)
	{
		UE_LOG(LogTinoAnimation, Warning, TEXT("%s: Loadout %s의 Locomotion 설정이 완전하지 않습니다."),
			*GetNameSafe(OwnerCharacter), *GetNameSafe(NewLoadout));
		return;
	}
	
	CurrentGroundMovement = Locomotion.GroundMovement;
	CurrentJump = Locomotion.Jump;
	CurrentFallLoop = Locomotion.FallLoop;
	CurrentLand = Locomotion.Land;
}
