// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "EnemyAttackNotify.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_TINOKINGDOM_API UEnemyAttackNotify : public UAnimNotify
{
	GENERATED_BODY()
	
	
public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;
};
