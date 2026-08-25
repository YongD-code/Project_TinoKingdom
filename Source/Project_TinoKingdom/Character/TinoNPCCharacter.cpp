// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoNPCCharacter.h"

#include "Camera/CameraComponent.h"

// Sets default values
ATinoNPCCharacter::ATinoNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	DialogueCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DialogueCamera"));
	DialogueCamera->SetupAttachment(GetRootComponent());

	DialogueCamera->SetRelativeLocation(FVector(DialogueCameraDistance, DialogueCameraSideOffset, DialogueCameraHeight));
	DialogueCamera->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
}

void ATinoNPCCharacter::FocusDialogueCamera()
{
	if (!IsValid(DialogueCamera))
	{
		return;
	}

	const FVector TargetLocation = GetActorLocation() + FVector(0.0f, 0.0f, DialogueTargetHeight);

	// NPC의 정면과 오른쪽을 수평면에 투영해 카메라 위치의 기준으로 삼는다.
	FVector CameraForwardDirection = GetActorForwardVector();
	CameraForwardDirection.Z = 0.0f;

	if (!CameraForwardDirection.Normalize())
	{
		CameraForwardDirection = FVector::ForwardVector;
	}

	FVector CameraRightDirection = GetActorRightVector();
	CameraRightDirection.Z = 0.0f;

	if (!CameraRightDirection.Normalize())
	{
		CameraRightDirection = FVector::RightVector;
	}

	const FVector CameraLocation =
		TargetLocation
		+ CameraForwardDirection * DialogueCameraDistance
		+ CameraRightDirection * DialogueCameraSideOffset
		+ FVector(0.0f, 0.0f, DialogueCameraHeight - DialogueTargetHeight);

	const FRotator CameraRotation = (TargetLocation - CameraLocation).Rotation();

	DialogueCamera->SetWorldLocation(CameraLocation);
	DialogueCamera->SetWorldRotation(CameraRotation);
}
