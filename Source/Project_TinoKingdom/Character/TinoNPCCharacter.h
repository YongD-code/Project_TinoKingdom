// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TinoNPCCharacter.generated.h"

class UCameraComponent;
class UDialogueData;

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATinoNPCCharacter();

	// 이 NPC가 사용할 대사 묶음. 대화 진행은 플레이어의 DialogueComponent가 담당한다.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	UDialogueData* GetDialogueData() const { return DialogueData; }

	// 대화 전용 카메라를 NPC 정면 구도로 배치한다.
	void FocusDialogueCamera();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> DialogueData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UCameraComponent> DialogueCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera")
	float DialogueCameraDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera")
	float DialogueCameraHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera")
	float DialogueCameraSideOffset = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera")
	float DialogueTargetHeight = 145.0f;
};
