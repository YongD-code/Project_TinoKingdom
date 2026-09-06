// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "FootstepAnimNotify.generated.h"

/**
 * 발이 바닥에 닿는 순간 현재 레벨의 발소리를 재생한다.
 * 소리는 애니메이션이 아니라 GameMode가 정하므로 레벨마다 다르게 할 수 있다.
 */
UCLASS(meta = (DisplayName = "Footstep"))
class PROJECT_TINOKINGDOM_API UFootstepAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	// 소리를 낼 발 소켓. 비워 두면 캐릭터 위치에서 난다.
	UPROPERTY(EditAnywhere, Category = "Footstep")
	FName FootSocketName;
};
