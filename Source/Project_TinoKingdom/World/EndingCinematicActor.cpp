#include "EndingCinematicActor.h"

#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Project_TinoKingdom/GameMode/TinoGameInstance.h"
#include "Project_TinoKingdom/Character/GuideNPCCharacter.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/World/EndingPortal.h"
#include "Project_TinoKingdom/Component/MagicStoneDestructionComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogEndingCinematic, Log, All);

namespace
{
	// SetBindingByTag은 태그를 못 찾아도 조용히 넘어가므로 미리 확인해 로그로 알린다.
	bool SequenceHasBindingTag(const ULevelSequence* Sequence, FName BindingTag)
	{
		const UMovieScene* MovieScene = Sequence ? Sequence->GetMovieScene() : nullptr;
		return MovieScene && MovieScene->AllTaggedBindings().Find(BindingTag) != nullptr;
	}

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

	if (bPlayAfterEndingTravel)
	{
		UTinoGameInstance* TinoGameInstance = Cast<UTinoGameInstance>(GetGameInstance());
		if (TinoGameInstance && TinoGameInstance->ConsumeEndingSequenceRequest())
		{
			UE_LOG(LogEndingCinematic, Log, TEXT("%s: 엔딩 이동을 확인했습니다."), *GetName());
			PlayEnding();
		}
		else
		{
			UE_LOG(LogEndingCinematic, Log, TEXT("%s: 엔딩 이동 요청이 없어 대기합니다."), *GetName());
		}
		return;
	}

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
		if (!SequenceHasBindingTag(EndingSequence, PlayerBindingTag))
		{
			UE_LOG(LogEndingCinematic, Error,
				TEXT("%s: 시퀀스에 '%s' 태그를 가진 바인딩이 없습니다. 시퀀서에서 태그를 확인하세요."),
				*GetName(), *PlayerBindingTag.ToString());
		}

		SequenceActor->SetBindingByTag(PlayerBindingTag, { PlayerCharacter });
		UE_LOG(LogEndingCinematic, Log, TEXT("Player 바인딩: %s"), *PlayerCharacter->GetName());

		// 시퀀서의 컨트롤 릭 포즈가 Leader Pose에 덮이지 않도록 연결을 끊는다.
		CinematicPlayerCharacter = PlayerCharacter;
		PlayerCharacter->SetCinematicPoseOverride(true);
	}

	if (IsValid(GuideNPC))
	{
		// 안내 AI가 살아 있으면 시네마틱 도중에 걸어가 버린다.
		GuideNPC->StopGuide();
		if (!SequenceHasBindingTag(EndingSequence, GuideBindingTag))
		{
			UE_LOG(LogEndingCinematic, Error,
				TEXT("%s: 시퀀스에 '%s' 태그를 가진 바인딩이 없습니다. 시퀀서에서 태그를 확인하세요."),
				*GetName(), *GuideBindingTag.ToString());
		}

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

	if (IsValid(CinematicPlayerCharacter))
	{
		CinematicPlayerCharacter->SetCinematicPoseOverride(false);
		CinematicPlayerCharacter = nullptr;
	}

	if (IsValid(SequenceActor))
	{
		SequenceActor->Destroy();
	}
	SequenceActor = nullptr;

	// 연출이 끝난 뒤에 포탈을 열어야 시네마틱 도중에 들어가는 일이 없다.
	if (IsValid(EndingPortal))
	{
		EndingPortal->RevealPortal();
	}

	UE_LOG(LogEndingCinematic, Log, TEXT("엔딩 시네마틱이 끝났습니다."));
}
