// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueComponent.h"

#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Project_TinoKingdom/Character/TinoNPCCharacter.h"
#include "Project_TinoKingdom/Component/TinoStateComponent.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"
#include "Project_TinoKingdom/DataAsset/DialogueData.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoDialogue, Log, All);

UDialogueComponent::UDialogueComponent()
{
	// 스킵 홀드 시간을 재는 동안에만 Tick이 필요하다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDialogueComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		StateComponent = Owner->FindComponentByClass<UTinoStateComponent>();
	}
}

void UDialogueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 대화 도중 레벨이 바뀌어도 시퀀스와 입력 컨텍스트가 남지 않도록 정리한다.
	if (IsInDialogue())
	{
		EndDialogue();
	}

	Super::EndPlay(EndPlayReason);
}

bool UDialogueComponent::IsInDialogue() const
{
	return CurrentNPC != nullptr;
}

bool UDialogueComponent::IsPlayingCinematic() const
{
	return CinematicPlayer != nullptr;
}

APlayerController* UDialogueComponent::GetOwningPlayerController() const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!IsValid(OwnerCharacter))
	{
		return nullptr;
	}

	return Cast<APlayerController>(OwnerCharacter->GetController());
}

bool UDialogueComponent::StartDialogue(ATinoNPCCharacter* InNPC)
{
	if (IsInDialogue() || !IsValid(InNPC))
	{
		return false;
	}

	UDialogueData* DialogueData = InNPC->GetDialogueData();

	if (!IsValid(DialogueData) || DialogueData->Lines.Num() == 0)
	{
		UE_LOG(LogTinoDialogue, Warning, TEXT("%s에 대화 데이터가 없어 대화를 시작하지 않는다."), *InNPC->GetName());
		return false;
	}

	APlayerController* PlayerController = GetOwningPlayerController();

	if (PlayerController == nullptr)
	{
		return false;
	}

	CurrentNPC = InNPC;
	CurrentDialogueData = DialogueData;
	CurrentLineIndex = 0;

	if (IsValid(StateComponent))
	{
		StateComponent->AddStateTag(TinoGameplayTags::State_InDialogue);
	}

	ApplyDialogueInputContext(true);

	PreviousViewTarget = PlayerController->GetViewTarget();
	InNPC->FocusDialogueCamera();
	PlayerController->SetViewTargetWithBlend(InNPC, CameraBlendTime);

	OnDialogueStarted.Broadcast();
	BroadcastCurrentLine();

	return true;
}

void UDialogueComponent::OnAdvancePressed()
{
	if (!IsInDialogue())
	{
		return;
	}

	// 시네마틱 중에는 바로 넘기지 않고 길게 눌러야 스킵된다.
	if (IsPlayingCinematic())
	{
		bSkipHoldActive = true;
		SkipHoldElapsed = 0.f;
		SetComponentTickEnabled(true);
		return;
	}

	AdvanceToNextLine();
}

void UDialogueComponent::OnAdvanceReleased()
{
	if (!bSkipHoldActive)
	{
		return;
	}

	bSkipHoldActive = false;
	SkipHoldElapsed = 0.f;
	SetComponentTickEnabled(false);
	OnDialogueSkipProgress.Broadcast(0.f);
}

void UDialogueComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bSkipHoldActive)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// 홀드 도중 시네마틱이 스스로 끝났다면 스킵할 대상이 없다.
	if (!IsPlayingCinematic())
	{
		OnAdvanceReleased();
		return;
	}

	SkipHoldElapsed += DeltaTime;
	OnDialogueSkipProgress.Broadcast(FMath::Clamp(SkipHoldElapsed / SkipHoldDuration, 0.f, 1.f));

	if (SkipHoldElapsed < SkipHoldDuration)
	{
		return;
	}

	bSkipHoldActive = false;
	SkipHoldElapsed = 0.f;
	SetComponentTickEnabled(false);
	OnDialogueSkipProgress.Broadcast(0.f);

	// 시퀀스를 끊고 대화를 이어간다.
	ClearCinematicPlayer();
	AdvanceToNextLine();
}

void UDialogueComponent::BroadcastCurrentLine()
{
	if (!IsValid(CurrentDialogueData) || !CurrentDialogueData->Lines.IsValidIndex(CurrentLineIndex))
	{
		return;
	}

	const FDialogueLine& Line = CurrentDialogueData->Lines[CurrentLineIndex];

	// 플레이어 대사는 NPC 이름표를 쓰지 않는다.
	const FText SpeakerName = (Line.Speaker == EDialogueSpeaker::NPC)
		? CurrentDialogueData->SpeakerDisplayName
		: FText::GetEmpty();

	// UI를 붙이기 전에도 진행 상황을 확인할 수 있도록 남긴다.
	UE_LOG(LogTinoDialogue, Log, TEXT("[%d/%d] %s: %s"),
		CurrentLineIndex + 1,
		CurrentDialogueData->Lines.Num(),
		*SpeakerName.ToString(),
		*Line.Text.ToString());

	OnDialogueLineChanged.Broadcast(SpeakerName, Line.Text);
}

void UDialogueComponent::AdvanceToNextLine()
{
	if (!IsValid(CurrentDialogueData))
	{
		EndDialogue();
		return;
	}

	// 현재 대사에 시네마틱이 걸려 있으면 먼저 재생하고, 끝난 뒤 다시 이 함수로 돌아온다.
	if (TryPlayCinematicForCurrentLine())
	{
		return;
	}

	++CurrentLineIndex;

	if (!CurrentDialogueData->Lines.IsValidIndex(CurrentLineIndex))
	{
		EndDialogue();
		return;
	}

	BroadcastCurrentLine();
}

bool UDialogueComponent::TryPlayCinematicForCurrentLine()
{
	if (!CurrentDialogueData->Lines.IsValidIndex(CurrentLineIndex))
	{
		return false;
	}

	const FDialogueLine& Line = CurrentDialogueData->Lines[CurrentLineIndex];

	if (Line.CinematicAfterLine.IsNull())
	{
		return false;
	}

	ULevelSequence* Sequence = Line.CinematicAfterLine.LoadSynchronous();

	if (!IsValid(Sequence))
	{
		UE_LOG(LogTinoDialogue, Warning, TEXT("대사 %d의 시네마틱을 불러오지 못했다."), CurrentLineIndex);
		return false;
	}

	ALevelSequenceActor* OutActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), Sequence, FMovieSceneSequencePlaybackSettings(), OutActor);

	if (Player == nullptr)
	{
		return false;
	}

	CinematicPlayer = Player;
	CinematicActor = OutActor;

	Player->OnFinished.AddDynamic(this, &UDialogueComponent::HandleCinematicFinished);
	Player->Play();

	return true;
}

void UDialogueComponent::HandleCinematicFinished()
{
	ClearCinematicPlayer();

	// 시네마틱을 유발한 대사는 소비된 것으로 보고 다음 대사로 넘어간다.
	++CurrentLineIndex;

	if (!IsValid(CurrentDialogueData) || !CurrentDialogueData->Lines.IsValidIndex(CurrentLineIndex))
	{
		EndDialogue();
		return;
	}

	BroadcastCurrentLine();
}

void UDialogueComponent::ClearCinematicPlayer()
{
	if (CinematicPlayer != nullptr)
	{
		// 종료 콜백이 다시 들어오지 않도록 먼저 구독을 끊는다.
		CinematicPlayer->OnFinished.RemoveDynamic(this, &UDialogueComponent::HandleCinematicFinished);
		CinematicPlayer->Stop();
		CinematicPlayer = nullptr;
	}

	if (IsValid(CinematicActor))
	{
		CinematicActor->Destroy();
	}

	CinematicActor = nullptr;
}

void UDialogueComponent::CancelDialogue()
{
	if (!IsInDialogue())
	{
		return;
	}

	EndDialogue();
}

void UDialogueComponent::EndDialogue()
{
	ClearCinematicPlayer();

	bSkipHoldActive = false;
	SkipHoldElapsed = 0.f;
	SetComponentTickEnabled(false);
	OnDialogueSkipProgress.Broadcast(0.f);

	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		if (IsValid(PreviousViewTarget))
		{
			PlayerController->SetViewTargetWithBlend(PreviousViewTarget, CameraBlendTime);
		}
	}

	ApplyDialogueInputContext(false);

	if (IsValid(StateComponent))
	{
		StateComponent->RemoveStateTag(TinoGameplayTags::State_InDialogue);
	}

	CurrentNPC = nullptr;
	CurrentDialogueData = nullptr;
	PreviousViewTarget = nullptr;
	CurrentLineIndex = INDEX_NONE;

	OnDialogueEnded.Broadcast();
}

void UDialogueComponent::ApplyDialogueInputContext(bool bEnable)
{
	if (DialogueMappingContext == nullptr)
	{
		return;
	}

	const APlayerController* PlayerController = GetOwningPlayerController();

	if (PlayerController == nullptr)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

	if (LocalPlayer == nullptr)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (InputSubsystem == nullptr)
	{
		return;
	}

	if (bEnable)
	{
		InputSubsystem->AddMappingContext(DialogueMappingContext, DialogueContextPriority);
		return;
	}

	InputSubsystem->RemoveMappingContext(DialogueMappingContext);
}
