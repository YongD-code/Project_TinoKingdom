#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "Project_TinoKingdom/Character/TinoNPCCharacter.h"
#include "GuideNPCCharacter.generated.h"

class AAIController;
class APlayerCharacter;
class UNiagaraSystem;

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

	// 엔딩 시네마틱에서 물짱이가 사람 형태로 바뀐다. 시퀀서 Event Track에서 호출한다.
	// 시퀀서가 건드린 값이 아니라 코드가 바꾼 상태이므로 시퀀스가 끝나도 유지된다.
	UFUNCTION(BlueprintCallable, Category = "Guide|Form")
	void SetEvolvedForm(bool bEvolved);

	UFUNCTION(BlueprintPure, Category = "Guide|Form")
	bool IsEvolvedForm() const { return bEvolvedForm; }

	virtual void OnDialogueCompleted_Implementation(APlayerCharacter* PlayerCharacter) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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
	float PlayerWaitDistance = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guide|Movement", meta = (ClampMin = "0.0"))
	float NavigationProjectionExtent = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guide|Movement", meta = (ClampMin = "0.05"))
	float MoveRequestInterval = 0.5f;

	// 두 형태를 컴포넌트 태그로 구분한다. 스켈레탈 메시든 차일드 액터든 태그만 맞으면 된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guide|Form")
	FName BaseFormMeshTag = TEXT("BaseForm");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guide|Form")
	FName EvolvedFormMeshTag = TEXT("EvolvedForm");

	// 변신하는 순간 터뜨릴 이펙트. 되돌릴 때는 재생하지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guide|Form")
	TObjectPtr<UNiagaraSystem> EvolveEffect;

	// 물고기와 사람의 키 차이를 보정해 이펙트를 띄울 높이.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guide|Form")
	float EvolveEffectHeightOffset = 60.0f;

private:
	void MoveToCurrentTarget();
	void AdvanceGuideTarget();
	bool IsPlayerCloseEnough() const;
	bool IsGuideReady() const;
	bool ResolveCurrentTargetLocation(FVector& OutTargetLocation) const;
	AAIController* ResolveGuideController();
	void BindMoveCompleted();
	void UnbindMoveCompleted();
	AActor* GetCurrentTarget() const;

	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UPROPERTY(Transient)
	TObjectPtr<APlayerCharacter> GuidedPlayer;

	UPROPERTY(Transient)
	TObjectPtr<AAIController> BoundGuideController;

	int32 CurrentTargetIndex = INDEX_NONE;
	bool bGuiding = false;
	bool bMoveRequestActive = false;
	float MoveRequestElapsed = 0.0f;
	bool bEvolvedForm = false;
};
