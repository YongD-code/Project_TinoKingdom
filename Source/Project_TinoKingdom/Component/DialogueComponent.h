// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogueComponent.generated.h"

class ACharacter;
class ALevelSequenceActor;
class APlayerController;
class ATinoNPCCharacter;
class UDialogueData;
class UInputMappingContext;
class ULevelSequencePlayer;
class UQuestComponent;
class UTinoStateComponent;

// UI가 C++을 직접 참조하지 않고 대화 상태에 반응할 수 있도록 델리게이트로 알린다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueLineChanged, const FText&, SpeakerName, const FText&, LineText);

// 시네마틱 스킵 홀드 진행도(0~1). 홀드를 놓거나 완료되면 0으로 되돌아온다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueSkipProgress, float, Progress);

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDialogueComponent();

	// 지정한 NPC와 대화를 시작한다. 이미 대화 중이면 아무것도 하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool StartDialogue(ATinoNPCCharacter* InNPC);

	// 대화를 즉시 중단하고 원래 상태로 되돌린다. (ESC)
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void CancelDialogue();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsInDialogue() const;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsPlayingCinematic() const;

	// 진행 입력을 누른 순간. 시네마틱 중이면 스킵 홀드를 시작한다.
	void OnAdvancePressed();

	// 진행 입력을 뗀 순간. 스킵 홀드를 취소한다.
	void OnAdvanceReleased();

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueStarted OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueEnded OnDialogueEnded;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueLineChanged OnDialogueLineChanged;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueSkipProgress OnDialogueSkipProgress;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 현재 대사를 화면에 반영한다.
	void BroadcastCurrentLine();

	// 다음 대사로 넘어간다. 남은 대사가 없으면 대화를 끝낸다.
	void AdvanceToNextLine();

	// 현재 대사에 지정된 시네마틱이 있으면 재생하고 true를 반환한다.
	bool TryPlayCinematicForCurrentLine();

	// 재생 중인 시퀀스를 정리한다. 종료 콜백은 호출되지 않는다.
	void ClearCinematicPlayer();

	// 대화를 종료하고 입력·카메라·상태를 대화 시작 전으로 되돌린다.
	void EndDialogue();

	// 대화가 끝난 NPC의 퀘스트를 수령하거나 완료 처리한다.
	void ResolveQuest(ATinoNPCCharacter* NPC);

	// 대화 전용 매핑 컨텍스트를 얹거나 걷어낸다.
	void ApplyDialogueInputContext(bool bEnable);

	APlayerController* GetOwningPlayerController() const;

	UFUNCTION()
	void HandleCinematicFinished();

private:
	// 대화 중 활성화할 입력 매핑 컨텍스트. IMC_Dialogue를 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DialogueMappingContext;

	// 기본 매핑 컨텍스트(0)보다 높아야 대화 입력이 우선한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Input", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 DialogueContextPriority = 1;

	// 대화 카메라로 전환하고 되돌아올 때의 블렌드 시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraBlendTime = 0.7f;

	// 시네마틱을 스킵하기 위해 진행 입력을 눌러야 하는 시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Cinematic", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float SkipHoldDuration = 2.0f;

	UPROPERTY(Transient)
	TObjectPtr<ATinoNPCCharacter> CurrentNPC;

	UPROPERTY(Transient)
	TObjectPtr<UDialogueData> CurrentDialogueData;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviousViewTarget;

	UPROPERTY(Transient)
	TObjectPtr<UTinoStateComponent> StateComponent;

	UPROPERTY(Transient)
	TObjectPtr<UQuestComponent> QuestComponent;

	// 재생 중인 시퀀스와 그 시퀀스를 소유한 액터.
	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> CinematicPlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> CinematicActor;

	int32 CurrentLineIndex = INDEX_NONE;

	bool bSkipHoldActive = false;
	float SkipHoldElapsed = 0.f;
};
