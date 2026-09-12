#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndingCinematicActor.generated.h"

class AGuideNPCCharacter;
class ALevelSequenceActor;
class ULevelSequence;
class ULevelSequencePlayer;

UCLASS()
class PROJECT_TINOKINGDOM_API AEndingCinematicActor : public AActor
{
	GENERATED_BODY()

public:
	AEndingCinematicActor();

	// 마력석 파괴와 무관하게 직접 재생하고 싶을 때 쓴다. 디버그와 블루프린트 테스트용.
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void PlayEnding();

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
	bool bPlayOnlyOnce = true;

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
	bool bPlayed = false;
};
