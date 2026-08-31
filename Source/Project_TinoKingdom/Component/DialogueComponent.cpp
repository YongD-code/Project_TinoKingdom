// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueComponent.h"

#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Engine/LocalPlayer.h"
#include "Project_TinoKingdom/Character/TinoNPCCharacter.h"
#include "Project_TinoKingdom/Component/QuestComponent.h"
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

	// 퀘스트 진행 상태에 따라 다른 대사를 고른다.
	QuestComponent = GetOwner() ? GetOwner()->FindComponentByClass<UQuestComponent>() : nullptr;

	UDialogueData* DialogueData = InNPC->SelectDialogueData(QuestComponent);

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

	ClearCinematicPlayer();
	AdvanceAfterCinematic();
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

	// NPC 대사일 때만 말하는 동작을 재생한다.
	if (Line.Speaker == EDialogueSpeaker::NPC && IsValid(CurrentNPC))
	{
		CurrentNPC->PlayTalkAnimation();
	}
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
		CompleteDialogue();
		return;
	}

	BroadcastCurrentLine();
}

void UDialogueComponent::AdvanceAfterCinematic()
{
	// 시네마틱이 연결되어 있던 현재 대사를 소비한다.
	++CurrentLineIndex;

	if (!IsValid(CurrentDialogueData))
	{
		EndDialogue();
		return;
	}

	if (!CurrentDialogueData->Lines.IsValidIndex(CurrentLineIndex))
	{
		CompleteDialogue();
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

	// 시네마틱이 직접 연기를 담당하므로 대화용 몸짓은 정리한다.
	if (IsValid(CurrentNPC))
	{
		CurrentNPC->StopTalkAnimation();
	}

	CinematicPlayer = Player;
	CinematicActor = OutActor;

	Player->OnFinished.AddDynamic(this, &UDialogueComponent::HandleCinematicFinished);
	Player->Play();

	return true;
}

void UDialogueComponent::HandleCinematicFinished()
{
	// 현재 OnFinished.Broadcast()가 실행 중이므로
	// 여기서 RemoveDynamic()이나 Stop()을 호출하면 안 된다.
	CinematicPlayer = nullptr;

	if (IsValid(CinematicActor))
	{
		// Destroy는 즉시 메모리를 제거하지 않고 안전하게 제거 예약을 한다.
		CinematicActor->Destroy();
	}

	CinematicActor = nullptr;

	AdvanceAfterCinematic();
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

void UDialogueComponent::CompleteDialogue()
{
	if (!IsInDialogue())
	{
		return;
	}
	
	ATinoNPCCharacter* CompletedNPC = CurrentNPC;
	
	EndDialogue();
	ResolveQuest(CompletedNPC);
}

void UDialogueComponent::EndDialogue()
{
	ClearCinematicPlayer();

	if (IsValid(CurrentNPC))
	{
		CurrentNPC->StopTalkAnimation();
	}

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

void UDialogueComponent::ResolveQuest(ATinoNPCCharacter* NPC)
{
	if (!IsValid(NPC) || !IsValid(QuestComponent))
	{
		return;
	}

	UQuestData* Quest = NPC->GetQuestToGrant();

	if (!IsValid(Quest))
	{
		return;
	}

	switch (QuestComponent->GetQuestState(Quest))
	{
	case EQuestState::NotStarted:
		QuestComponent->AcceptQuest(Quest);
		break;
	case EQuestState::ReadyToComplete:
		QuestComponent->CompleteQuest(Quest);
		break;
	default:
		// 진행 중이거나 이미 끝난 퀘스트는 대화만 하고 상태를 바꾸지 않는다.
		break;
	}
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
