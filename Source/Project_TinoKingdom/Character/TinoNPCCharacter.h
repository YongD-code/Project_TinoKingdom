// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Project_TinoKingdom/Interface/TargetableInterface.h"
#include "TinoNPCCharacter.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UTinoAbilitySystemComponent;
class UTinoAttributeSet;
class UAnimMontage;
class UCameraComponent;
class UDialogueData;
class UQuestComponent;
class UQuestData;
class USkeletalMeshComponent;

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoNPCCharacter :	public ACharacter,
													public ITargetableInterface,
													public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATinoNPCCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	const UTinoAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		class AController* EventInstigator, AActor* DamageCauser) override;
	
	virtual bool CanBeTargeted_Implementation() const override;
	virtual FVector GetLockOnLocation_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "NPC|State")
	bool IsDead() const;
	
	UFUNCTION(BlueprintPure, Category = "NPC|Combat")
	AActor* GetCombatTarget() const { return CombatTarget.Get(); }
	
	// 이 NPC가 사용할 대사 묶음. 대화 진행은 플레이어의 DialogueComponent가 담당한다.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	UDialogueData* GetDialogueData() const { return DialogueData; }

	// 퀘스트 진행 상태에 맞는 대사를 고른다. 해당 상태의 대사가 없으면 기본 대사를 쓴다.
	UDialogueData* SelectDialogueData(const UQuestComponent* PlayerQuest) const;

	UFUNCTION(BlueprintPure, Category = "Quest")
	UQuestData* GetQuestToGrant() const { return QuestToGrant; }

	// 대화 전용 카메라를 NPC 정면 구도로 배치한다.
	void FocusDialogueCamera();

	// 대사 한 줄을 말하는 동안 재생할 몸짓과 표정. 몽타주가 끝나면 원래 자세로 돌아온다.
	void PlayTalkAnimation();

	// 대화가 끝나거나 시네마틱이 시작될 때 말하는 동작을 정리한다.
	void StopTalkAnimation();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 메타휴먼은 몸과 얼굴이 각각 다른 스켈레탈 메시라 이름으로 찾아 캐시한다.
	void CacheAnimationMeshes();

	// 카메라가 화면 중앙에 둘 지점. 머리 본을 찾으면 그 위치를 기준으로 한다.
	FVector GetDialogueFocusLocation() const;

	void PlayHitReaction();
	
	void HandleDeath();
	
	void StartRespawnCheck();
	void CheckRespawnCondition();
	
	bool IsInsidePlayerViewFrustum() const;
	
	void RespawnAtInitialTransform();
	
	AActor* ResolveDamageInstigator(AController* EventInstigator, AActor* DamageCauser) const;
	void SetCombatTarget(AActor* NewTarget);

	static void PlayMontageOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage);
	static void StopMontageOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage, float BlendOutTime);

	bool InitializeDefaultAttributes();
	
	float ApplyDamageGameplayEffect(float DamageAmount, AController* EventInstigator, AActor* DamageCauser);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UTinoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UTinoAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability System")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability System")
	TSubclassOf<UGameplayEffect> DamageEffect;
	
	// 퀘스트를 받기 전에 할 대사.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> DialogueData;

	// 퀘스트를 받았지만 아직 목표를 못 채웠을 때의 대사.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> InProgressDialogueData;

	// 목표를 다 채워 보고하러 왔을 때의 대사. 예) 정말 고맙네!
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> ReadyToCompleteDialogueData;

	// 퀘스트를 끝낸 뒤 다시 말을 걸었을 때의 대사.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> CompletedDialogueData;

	// 이 NPC가 건네줄 퀘스트. 대화가 끝나는 시점에 수령된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TObjectPtr<UQuestData> QuestToGrant;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UCameraComponent> DialogueCamera;

	// NPC 정면에서 카메라까지의 거리. 작을수록 얼굴이 크게 잡힌다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera", meta = (ClampMin = "20.0"))
	float DialogueCameraDistance = 110.0f;

	// 옆으로 비껴선 정도. 0이면 정면, 값이 클수록 비스듬한 구도가 된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera")
	float DialogueCameraSideOffset = 25.0f;

	// 머리 본에서 아래로 내린 만큼이 화면 중앙이 된다. 음수면 얼굴이 위쪽에 오고 상체가 함께 잡힌다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera")
	float DialogueFocusHeightOffset = -12.0f;

	// 좁을수록 배경이 압축되고 인물이 도드라진다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera", meta = (ClampMin = "10.0", ClampMax = "120.0"))
	float DialogueCameraFOV = 55.0f;

	// 조준할 머리 본 이름. 메타휴먼은 head를 쓴다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Camera")
	FName HeadBoneName = TEXT("head");

	// 머리 본을 찾지 못했을 때 쓰는 액터 기준 높이.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera")
	float DialogueTargetHeight = 80.0f;

	// 대사마다 재생할 몸짓. 비워두면 몸은 가만히 있는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Animation")
	TObjectPtr<UAnimMontage> TalkBodyMontage;

	// 대사마다 재생할 표정. 비워두면 표정 변화가 없다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Animation")
	TObjectPtr<UAnimMontage> TalkFaceMontage;

	// 생존 가능한 피해를 받았을 때 몸 메시에서 재생할 피격 몽타주.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Animation|Hit")
	TObjectPtr<UAnimMontage> HitBodyMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Animation|Death")
	TObjectPtr<UAnimMontage> DeathBodyMontage;

	// 사망한 NPC가 이 시간 동안 연속으로 화면 밖에 있으면 부활
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Respawn", meta = (ClampMin = "0.0"))
	float OutOfViewRespawnDelay = 3.f;

	// 사망한 NPC가 화면 안에 있는지 검사하는 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Respawn", meta = (ClampMin = "0.05"))
	float RespawnVisibilityCheckInterval = 0.25f;
	
	// 메타휴먼 블루프린트의 컴포넌트 이름. 다른 이름을 쓰는 NPC는 여기서 바꾼다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Animation")
	FName BodyMeshComponentName = TEXT("Body");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Animation")
	FName FaceMeshComponentName = TEXT("Face");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Animation", meta = (ClampMin = "0.0"))
	float TalkMontageBlendOutTime = 0.25f;

private:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> FaceMesh;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CombatTarget;
	
private:
	FTransform InitialSpawnTransform = FTransform::Identity;
	
	FTimerHandle RespawnCheckTimerHandle;
	
	double OutOfViewStartTime = -1.0;
};
