// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DodgeComponent.generated.h"

class UTinoStateComponent;
class ACharacter;
class UAnimMontage;

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UDodgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDodgeComponent();
	
	bool StartDodge(const FVector& DodgeDirection, bool bUseStrafeDodge);
	void CancelDodge();
	
	void BeginInvincibilityWindow();
	void EndInvincibilityWindow();

protected:
	virtual void BeginPlay() override;
	
private:
	FName SelectStrafeDodgeSection(const FVector& DodgeDirection);
	void FinishDodge();
	
	void OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge", meta = (AllowPrivateAccess = true))
	TObjectPtr<UAnimMontage> DodgeMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> StrafeDodgeMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDodgeMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;
	
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> AnimationMesh;
	
	UPROPERTY(Transient)
	TObjectPtr<UTinoStateComponent> StateComponent;
	
	UPROPERTY(Transient)
	bool bInvincibilityWindowActive = false;
};
