// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TinoNPCCharacter.generated.h"

class UCameraComponent;

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATinoNPCCharacter();

protected:
	// 게임시작 또는 호출되었을때 실행
	virtual void BeginPlay() override;
	
	UPROPERTY()
	APlayerController* DialoguePlayerController;

	UPROPERTY()
	AActor* PreviousViewTarget;

	UPROPERTY()
	int32 CurrentDialogueIndex = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue")
	UCameraComponent* DialogueCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Camera")
	float DialogueCameraDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Camera")
	float DialogueCameraHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Camera")
	float DialogueCameraSideOffset = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue|Camera")
	float DialogueTargetHeight = 145.0f;

	UPROPERTY()
	TArray<FText> CurrentDialogueLines;

public:	
	// 매프레임 실행
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	void AdvanceDialogue();
	
	UFUNCTION(BlueprintCallable)
	virtual void StartDialogue(APlayerController* PlayerController);
	
	UFUNCTION(BlueprintCallable)
	virtual void EndDialogue();
	
	virtual TArray<FText> GetDialogueLines() const;
	
};
