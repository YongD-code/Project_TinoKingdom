// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TinoAnimInstance.generated.h"

/**
 * 
 */

class ACharacter;
class UCharacterMovementComponent;

UCLASS()
class PROJECT_TINOKINGDOM_API UTinoAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UTinoAnimInstance();
	
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
private:
	// 런타임에 한 번 찾아서 캐시하는 참조
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent = nullptr;
	
	// 매 프레임마다 계산되는 애니메이션 입력값
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	uint8 bIsIdle : 1;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	uint8 bIsFalling : 1;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	uint8 bIsJumping : 1;
	
	// AnimBP Class Defaults에서 설정하고 저장할 수 있는 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Settings", meta = (AllowPrivateAccess = "true"))
	float MovingSpeedThreshold = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Settings", meta = (AllowPrivateAccess = "true"))
	float JumpingSpeedThreshold = 0.f;
};
