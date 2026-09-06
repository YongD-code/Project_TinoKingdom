#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossIntroCinematicActor.generated.h"

class AEnemyCharacter;
class ALevelSequenceActor;
class APlayerCharacter;
class ATinoNPCCharacter;
class ULevelSequence;
class ULevelSequencePlayer;

UCLASS()
class PROJECT_TINOKINGDOM_API ABossIntroCinematicActor : public AActor
{
	GENERATED_BODY()

public:
	ABossIntroCinematicActor();

	UFUNCTION(BlueprintCallable, Category = "Boss Intro")
	void PlayIntro(APlayerCharacter* PlayerCharacter);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Boss Intro")
	TObjectPtr<ATinoNPCCharacter> BossDialogueNPC;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Boss Intro")
	TObjectPtr<AEnemyCharacter> BossEnemy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Intro")
	TObjectPtr<ULevelSequence> IntroSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Intro")
	bool bPlayAfterBossDialogue = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Intro")
	bool bHideBossEnemyUntilIntroFinishes = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Intro")
	bool bHideDialogueNPCWhenFightStarts = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Intro")
	bool bDisablePlayerDuringIntro = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Intro")
	bool bPlayOnlyOnce = true;

private:
	UFUNCTION()
	void HandleBossDialogueCompleted(ATinoNPCCharacter* NPC, APlayerCharacter* PlayerCharacter);

	UFUNCTION()
	void HandleIntroFinished();

	void PrepareBossEnemyForIntro();
	void StartBossFight();
	void SetPlayerCinematicMode(bool bEnabled) const;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> IntroSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> IntroSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<APlayerCharacter> CurrentPlayer;

	bool bIntroPlayed = false;
};
