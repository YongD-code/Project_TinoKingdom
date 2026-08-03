// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReactionComponent.generated.h"

UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Front,
	Back,
	Left,
	Right
};

class AActor;
class ACharacter;
class UAnimMontage;
class USkeletalMeshComponent;
class UTinoCombatComponent;

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UReactionComponent();
	
	// DamageCauser 위치를 기준으로 방향을 계산하고 그에 맞는 Hit Reaction 재생
	UFUNCTION(BlueprintCallable, Category = "Reaction")
	bool PlayHitReaction(AActor* DamageCauser);
	
	UFUNCTION(BlueprintPure, Category = "Reaction")
	bool IsReacting() const { return bIsReacting; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
private:
	// 공격자가 피격자의 어느 방향에 있는지 계산
	EHitDirection CalculateHitDirection(const AActor* DamageCauser);
	
	// 방향에 해당하는 피격 몽타주 반환
	UAnimMontage* GetHitMontage(EHitDirection HitDirection) const;
	
	// 현재 피격 몽타주가 종료되거나 중단되었을 때 상태 초기화
	// TinoCombatComponent와 동일
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction|Hit", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> HitFromFrontMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction|Hit", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> HitFromBackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction|Hit", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> HitFromLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction|Hit", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> HitFromRightMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;
	
	// 몽타주를 실제로 플레이하는 Driver Mesh
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> AnimationMesh;
	
	// 피격시 현재 공격 상태를 취소하기 위해 캐싱
	UPROPERTY(Transient)
	TObjectPtr<UTinoCombatComponent> CombatComponent;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveHitMontage;
	
	UPROPERTY(Transient)
	bool bIsReacting = false;
};
