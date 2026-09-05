#pragma once

#include "CoreMinimal.h"
#include "Project_TinoKingdom/Character/TinoNPCCharacter.h"
#include "GuideNPCCharacter.generated.h"

class AAIController;
class APlayerCharacter;

UCLASS()
class PROJECT_TINOKINGDOM_API AGuideNPCCharacter : public ATinoNPCCharacter
{
	GENERATED_BODY()

public:
	AGuideNPCCharacter();

	UFUNCTION(BlueprintCallable, Category = "Guide")
	void StartGuide(APlayerCharacter* PlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "Guide")
	void StopGuide();

	UFUNCTION(BlueprintPure, Category = "Guide")
	bool IsGuiding() const { return bGuiding; }

	virtual void OnDialogueCompleted_Implementation(APlayerCharacter* PlayerCharacter) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guide")
	TArray<TObjectPtr<AActor>> GuideTargets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guide")
	bool bStartGuideAfterDialogue = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guide")
	bool bLoopGuide = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guide|Movement", meta = (ClampMin = "0.0"))
	float GuideWalkSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guide|Movement", meta = (ClampMin = "0.0"))
	float TargetAcceptanceRadius = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guide|Movement", meta = (ClampMin = "0.0"))
	float PlayerWaitDistance = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guide|Movement", meta = (ClampMin = "0.05"))
	float MoveRequestInterval = 0.5f;

private:
	void MoveToCurrentTarget();
	void AdvanceGuideTarget();
	bool IsPlayerCloseEnough() const;
	AActor* GetCurrentTarget() const;

	UPROPERTY(Transient)
	TObjectPtr<APlayerCharacter> GuidedPlayer;

	int32 CurrentTargetIndex = INDEX_NONE;
	bool bGuiding = false;
	float MoveRequestElapsed = 0.0f;
};
