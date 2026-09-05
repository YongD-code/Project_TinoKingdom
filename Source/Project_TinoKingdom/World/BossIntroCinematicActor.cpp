#include "BossIntroCinematicActor.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Character/TinoNPCCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogBossIntroCinematic, Log, All);

ABossIntroCinematicActor::ABossIntroCinematicActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABossIntroCinematicActor::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(BossDialogueNPC) && bPlayAfterBossDialogue)
	{
		BossDialogueNPC->OnNPCDialogueCompleted.AddUniqueDynamic(
			this,
			&ABossIntroCinematicActor::HandleBossDialogueCompleted);
	}

	PrepareBossEnemyForIntro();
}

void ABossIntroCinematicActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(BossDialogueNPC))
	{
		BossDialogueNPC->OnNPCDialogueCompleted.RemoveDynamic(
			this,
			&ABossIntroCinematicActor::HandleBossDialogueCompleted);
	}

	if (IsValid(IntroSequencePlayer))
	{
		IntroSequencePlayer->OnFinished.RemoveDynamic(this, &ABossIntroCinematicActor::HandleIntroFinished);
	}

	Super::EndPlay(EndPlayReason);
}

void ABossIntroCinematicActor::PlayIntro(APlayerCharacter* PlayerCharacter)
{
	if (bPlayOnlyOnce && bIntroPlayed)
	{
		return;
	}

	bIntroPlayed = true;
	CurrentPlayer = PlayerCharacter;
	PrepareBossEnemyForIntro();
	SetPlayerCinematicMode(true);

	if (!IsValid(IntroSequence))
	{
		UE_LOG(LogBossIntroCinematic, Warning, TEXT("%s: IntroSequence가 비어 있어 바로 전투를 시작합니다."), *GetName());
		HandleIntroFinished();
		return;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* CreatedSequenceActor = nullptr;
	IntroSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(),
		IntroSequence,
		PlaybackSettings,
		CreatedSequenceActor);
	IntroSequenceActor = CreatedSequenceActor;

	if (!IsValid(IntroSequencePlayer))
	{
		UE_LOG(LogBossIntroCinematic, Warning, TEXT("%s: LevelSequencePlayer 생성 실패."), *GetName());
		HandleIntroFinished();
		return;
	}

	IntroSequencePlayer->OnFinished.AddUniqueDynamic(this, &ABossIntroCinematicActor::HandleIntroFinished);
	IntroSequencePlayer->Play();
}

void ABossIntroCinematicActor::HandleBossDialogueCompleted(
	ATinoNPCCharacter* NPC,
	APlayerCharacter* PlayerCharacter)
{
	if (NPC != BossDialogueNPC)
	{
		return;
	}

	PlayIntro(PlayerCharacter);
}

void ABossIntroCinematicActor::HandleIntroFinished()
{
	if (IsValid(IntroSequencePlayer))
	{
		IntroSequencePlayer->OnFinished.RemoveDynamic(this, &ABossIntroCinematicActor::HandleIntroFinished);
	}

	SetPlayerCinematicMode(false);
	StartBossFight();
}

void ABossIntroCinematicActor::PrepareBossEnemyForIntro()
{
	if (!bHideBossEnemyUntilIntroFinishes || !IsValid(BossEnemy))
	{
		return;
	}

	BossEnemy->SetActorHiddenInGame(true);
	BossEnemy->SetActorEnableCollision(false);
	BossEnemy->SetActorTickEnabled(false);

	if (AAIController* AIController = Cast<AAIController>(BossEnemy->GetController()))
	{
		AIController->StopMovement();

		if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
		{
			BrainComponent->PauseLogic(TEXT("Waiting for boss intro"));
		}
	}

	if (UCharacterMovementComponent* MovementComponent = BossEnemy->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}
}

void ABossIntroCinematicActor::StartBossFight()
{
	if (IsValid(BossDialogueNPC) && bHideDialogueNPCWhenFightStarts)
	{
		BossDialogueNPC->SetActorHiddenInGame(true);
		BossDialogueNPC->SetActorEnableCollision(false);
		BossDialogueNPC->SetActorTickEnabled(false);
	}

	if (!IsValid(BossEnemy))
	{
		return;
	}

	BossEnemy->SetActorHiddenInGame(false);
	BossEnemy->SetActorEnableCollision(true);
	BossEnemy->SetActorTickEnabled(true);
	BossEnemy->SpawnDefaultController();

	if (UCharacterMovementComponent* MovementComponent = BossEnemy->GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	if (AAIController* AIController = Cast<AAIController>(BossEnemy->GetController()))
	{
		if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
		{
			BrainComponent->ResumeLogic(TEXT("Boss intro finished"));
		}
	}

	if (IsValid(CurrentPlayer))
	{
		BossEnemy->SetAggroTarget(CurrentPlayer);
	}
}

void ABossIntroCinematicActor::SetPlayerCinematicMode(bool bEnabled) const
{
	if (!bDisablePlayerDuringIntro || !IsValid(CurrentPlayer))
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(CurrentPlayer->GetController()))
	{
		PlayerController->SetCinematicMode(bEnabled, bEnabled, bEnabled, true, true);
	}
}
