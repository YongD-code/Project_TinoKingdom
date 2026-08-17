// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoNPCCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

// Sets default values
ATinoNPCCharacter::ATinoNPCCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DialogueCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DialogueCamera"));
	DialogueCamera->SetupAttachment(GetRootComponent());

	DialogueCamera->SetRelativeLocation(FVector(DialogueCameraDistance, DialogueCameraSideOffset, DialogueCameraHeight));
	DialogueCamera->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

}

// Called when the game starts or when spawned
void ATinoNPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATinoNPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATinoNPCCharacter::AdvanceDialogue()
{
	CurrentDialogueIndex++;

	if (!CurrentDialogueLines.IsValidIndex(CurrentDialogueIndex))
	{
		EndDialogue();
		return;
	}

	//UpdateDialogueUI();
}

void ATinoNPCCharacter::StartDialogue(APlayerController* PlayerController)
{
	if (PlayerController == nullptr)
	{
		return;
	}

	DialoguePlayerController = PlayerController;
	PreviousViewTarget = PlayerController->GetViewTarget();

    if (DialogueCamera)
    {
        const FVector TargetLocation = GetActorLocation() + FVector(0.0f, 0.0f, DialogueTargetHeight);
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

	PlayerController->SetViewTargetWithBlend(this, 0.7f);
	
	CurrentDialogueLines = GetDialogueLines();
	CurrentDialogueIndex = 0;
	
	//ShowDialogueUI();
	//UpdateDialogueUI();

	if (CurrentDialogueLines.IsValidIndex(CurrentDialogueIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Dialogue: %s"),
			*CurrentDialogueLines[CurrentDialogueIndex].ToString());
	}
}

void ATinoNPCCharacter::EndDialogue()
{
	if (DialoguePlayerController && PreviousViewTarget)
	{
		DialoguePlayerController->SetViewTargetWithBlend(PreviousViewTarget, 0.7f);
	}

	DialoguePlayerController = nullptr;
	PreviousViewTarget = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("대화 종료"));
}

TArray<FText> ATinoNPCCharacter::GetDialogueLines() const
{
	return {
		FText::FromString(TEXT("더운날 고생이 많네"))
	};
}
