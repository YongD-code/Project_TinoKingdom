// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TinoNPCCharacter.generated.h"

class UAnimMontage;
class UCameraComponent;
class UDialogueData;
class USkeletalMeshComponent;

UCLASS()
class PROJECT_TINOKINGDOM_API ATinoNPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATinoNPCCharacter();

	// 이 NPC가 사용할 대사 묶음. 대화 진행은 플레이어의 DialogueComponent가 담당한다.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	UDialogueData* GetDialogueData() const { return DialogueData; }

	// 대화 전용 카메라를 NPC 정면 구도로 배치한다.
	void FocusDialogueCamera();

	// 대사 한 줄을 말하는 동안 재생할 몸짓과 표정. 몽타주가 끝나면 원래 자세로 돌아온다.
	void PlayTalkAnimation();

	// 대화가 끝나거나 시네마틱이 시작될 때 말하는 동작을 정리한다.
	void StopTalkAnimation();

protected:
	virtual void BeginPlay() override;

private:
	// 메타휴먼은 몸과 얼굴이 각각 다른 스켈레탈 메시라 이름으로 찾아 캐시한다.
	void CacheAnimationMeshes();

	// 카메라가 화면 중앙에 둘 지점. 머리 본을 찾으면 그 위치를 기준으로 한다.
	FVector GetDialogueFocusLocation() const;

	static void PlayMontageOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage);
	static void StopMontageOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage, float BlendOutTime);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> DialogueData;

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
};
