#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndingCinematicActor.generated.h"

class AEndingPortal;
class AGuideNPCCharacter;
class ATinoEndingCrowdController;
class UNiagaraSystem;
class ALevelSequenceActor;
class ULevelSequence;
class ULevelSequencePlayer;

UCLASS()
class PROJECT_TINOKINGDOM_API AEndingCinematicActor : public AActor
{
	GENERATED_BODY()

public:
	AEndingCinematicActor();

	// 마력석 파괴와 무관하게 직접 재생한다. Details 패널의 버튼으로도 부를 수 있다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ending")
	void PlayEnding();

	// 시퀀스 없이 군중 전환만 확인한다. 연기와 사람이 나오는지 볼 때 쓴다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ending|Crowd")
	void PlayCrowdCue();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
	TObjectPtr<ULevelSequence> EndingSequence;

	// 마력석 파편이 흩어지는 걸 보여줄 시간. 파괴 이벤트는 파편이 퍼지기 전에 온다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending", meta = (ClampMin = "0.0"))
	float StartDelay = 1.5f;

	// 마력석 액터. 여기 붙은 MagicStoneDestructionComponent의 파괴 이벤트를 구독한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending")
	TObjectPtr<AActor> MagicStoneActor;

	// 시퀀서에서 Guide 태그를 단 바인딩에 연결할 물짱이.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending")
	TObjectPtr<AGuideNPCCharacter> GuideNPC;

	// 시네마틱이 끝나면 열어줄 귀환 포탈. 비워두면 포탈 없이 끝난다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending")
	TObjectPtr<AEndingPortal> EndingPortal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
	bool bPlayOnlyOnce = true;

	// 엔딩 포탈로 이동해 온 경우에만 레벨 시작 시 재생한다.
	// 일반 게임 시작에는 요청 플래그가 없어 반응하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
	bool bPlayAfterEndingTravel = false;

	// 몬스터를 사람으로 바꿔줄 팀원의 컨트롤러. 비워두면 전환을 요청하지 않는다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending|Crowd")
	TObjectPtr<ATinoEndingCrowdController> CrowdController;

	// 재생 시작부터 전환을 요청하기까지의 시간.
	// 첫 컷이 화면에 뜬 뒤여야 카메라가 비추는 동안 바뀐다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Crowd", meta = (ClampMin = "0.0"))
	float CrowdSpawnDelay = 0.7f;

	// 몬스터가 사라지고 사람이 나타나기까지의 빈 프레임을 가려줄 연기.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Crowd")
	TObjectPtr<UNiagaraSystem> CrowdSmokeEffect;

	// 연기를 터뜨릴 대상. CrowdController의 Target Monster Tag와 같은 값이어야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending|Crowd")
	FName CrowdMonsterTag = TEXT("EndingCrowdTarget");

private:
	UFUNCTION()
	void HandleStoneBroken();

	UFUNCTION()
	void HandleEndingFinished();

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> SequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	// 재생 중 Leader Pose를 끊어둔 플레이어. 종료 시 되돌리기 위해 들고 있는다.
	UPROPERTY(Transient)
	TObjectPtr<class APlayerCharacter> CinematicPlayerCharacter;

	FTimerHandle StartTimerHandle;
	FTimerHandle CrowdTimerHandle;
	bool bPlayed = false;
};
