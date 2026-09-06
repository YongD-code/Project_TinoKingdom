// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "KingAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS()
class PROJECT_TINOKINGDOM_API UKingAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	void CacheOwnerReferences();
	void ResetLocomotionState();

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed = 0.0f;

	// 캐릭터 정면 기준 이동 방향. 전진 0, 오른쪽 90, 왼쪽 -90, 후진 180이다.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	float MovementDirection = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsIdle = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsMoving = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsFalling = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Settings", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MovingSpeedThreshold = 3.0f;
};
