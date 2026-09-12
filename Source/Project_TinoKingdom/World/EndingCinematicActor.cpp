#include "EndingCinematicActor.h"

#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/Character/GuideNPCCharacter.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Component/MagicStoneDestructionComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogEndingCinematic, Log, All);

namespace
{
	// 시퀀서 바인딩에 달아둔 태그와 문자열이 정확히 일치해야 한다.
	const FName PlayerBindingTag(TEXT("Player"));
	const FName GuideBindingTag(TEXT("Guide"));
}

AEndingCinematicActor::AEndingCinematicActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEndingCinematicActor::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(MagicStoneActor))
	{
		UE_LOG(LogEndingCinematic, Warning,
			TEXT("%s: MagicStoneActor가 비어 있어 엔딩이 시작되지 않습니다."), *GetName());
		return;
	}

	UMagicStoneDestructionComponent* Destruction =
		MagicStoneActor->FindComponentByClass<UMagicStoneDestructionComponent>();

	if (!IsValid(Destruction))
	{
		UE_LOG(LogEndingCinematic, Warning,
			TEXT("%s: %s에 MagicStoneDestructionComponent가 없습니다."),
			*GetName(), *MagicStoneActor->GetName());
		return;
	}

	Destruction->OnStoneBroken.AddUniqueDynamic(this, &AEndingCinematicActor::HandleStoneBroken);

	UE_LOG(LogEndingCinematic, Log, TEXT("%s: %s의 파괴 이벤트를 구독했습니다."),
		*GetName(), *MagicStoneActor->GetName());
}

void AEndingCinematicActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StartTimerHandle);

	if (IsValid(MagicStoneActor))
	{
		if (UMagicStoneDestructionComponent* Destruction =
			MagicStoneActor->FindComponentByClass<UMagicStoneDestructionComponent>())
		{
			Destruction->OnStoneBroken.RemoveDynamic(this, &AEndingCinematicActor::HandleStoneBroken);
		}
	}

	if (IsValid(SequencePlayer))
	{
		SequencePlayer->OnFinished.RemoveDynamic(this, &AEndingCinematicActor::HandleEndingFinished);
	}

	Super::EndPlay(EndPlayReason);
}

void AEndingCinematicActor::HandleStoneBroken()
{
	if (bPlayOnlyOnce && bPlayed)
	{
		return;
	}
	bPlayed = true;

	UE_LOG(LogEndingCinematic, Log, TEXT("%s: 마력석 파괴 감지. %.1f초 뒤 시작합니다."),
		*GetName(), StartDelay);

	if (StartDelay <= 0.0f)
	{
		PlayEnding();
		return;
	}

	GetWorldTimerManager().SetTimer(
		StartTimerHandle, this, &AEndingCinematicActor::PlayEnding, StartDelay, false);
}

void AEndingCinematicActor::PlayEnding()
{
	if (!IsValid(EndingSequence))
	{
		UE_LOG(LogEndingCinematic, Warning, TEXT("%s: EndingSequence가 비어 있습니다."), *GetName());
		return;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.bDisableMovementInput = true;
	PlaybackSettings.bDisableLookAtInput = true;
	PlaybackSettings.FinishCompletionStateOverride =
		EMovieSceneCompletionModeOverride::ForceRestoreState;

	ALevelSequenceActor* CreatedActor = nullptr;
	ULevelSequencePlayer* CreatedPlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), EndingSequence, PlaybackSettings, CreatedActor);

	if (!IsValid(CreatedPlayer) || !IsValid(CreatedActor))
	{
		if (IsValid(CreatedActor))
		{
			CreatedActor->Destroy();
		}
		UE_LOG(LogEndingCinematic, Warning, TEXT("%s: 시퀀스 플레이어 생성에 실패했습니다."), *GetName());
		return;
	}

	SequencePlayer = CreatedPlayer;
	SequenceActor = CreatedActor;

	// 에디터에서 잡아둔 임시 액터 대신 실제로 플레이 중인 액터를 물린다.
	// Play() 뒤에 바꾸면 첫 프레임이 임시 액터로 평가되므로 반드시 재생 전에 한다.
	if (APlayerCharacter* PlayerCharacter =
		Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		SequenceActor->SetBindingByTag(PlayerBindingTag, { PlayerCharacter });
		UE_LOG(LogEndingCinematic, Log, TEXT("Player 바인딩: %s"), *PlayerCharacter->GetName());
	}

	if (IsValid(GuideNPC))
	{
		// 안내 AI가 살아 있으면 시네마틱 도중에 걸어가 버린다.
		GuideNPC->StopGuide();
		SequenceActor->SetBindingByTag(GuideBindingTag, { GuideNPC });
		UE_LOG(LogEndingCinematic, Log, TEXT("Guide 바인딩: %s"), *GuideNPC->GetName());
	}

	SequencePlayer->OnFinished.AddUniqueDynamic(this, &AEndingCinematicActor::HandleEndingFinished);
	SequencePlayer->Play();

	UE_LOG(LogEndingCinematic, Log, TEXT("%s: %s 재생을 시작합니다."),
		*GetName(), *EndingSequence->GetName());
}

void AEndingCinematicActor::HandleEndingFinished()
{
	// OnFinished 브로드캐스트 중에는 델리게이트를 제거하거나 Stop을 호출하지 않는다.
	SequencePlayer = nullptr;

	if (IsValid(SequenceActor))
	{
		SequenceActor->Destroy();
	}
	SequenceActor = nullptr;

	UE_LOG(LogEndingCinematic, Log, TEXT("엔딩 시네마틱이 끝났습니다."));
}
