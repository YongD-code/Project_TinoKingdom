// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoNPCCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Project_TinoKingdom/Component/QuestComponent.h"
#include "Project_TinoKingdom/DataAsset/DialogueData.h"
#include "Project_TinoKingdom/DataAsset/QuestData.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoNPC, Log, All);

// Sets default values
ATinoNPCCharacter::ATinoNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	DialogueCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DialogueCamera"));
	DialogueCamera->SetupAttachment(GetRootComponent());

	// 실제 위치는 대화 시작 시 FocusDialogueCamera에서 머리 본 기준으로 다시 잡는다.
	// 여기 값은 에디터에서 대략적인 위치를 보여주기 위한 것이다.
	DialogueCamera->SetRelativeLocation(FVector(DialogueCameraDistance, DialogueCameraSideOffset, 60.0f));
	DialogueCamera->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	DialogueCamera->SetFieldOfView(DialogueCameraFOV);
}

void ATinoNPCCharacter::FocusDialogueCamera()
{
	if (!IsValid(DialogueCamera))
	{
		return;
	}
	
	const FVector FocusLocation = GetDialogueFocusLocation();

	// NPC의 정면과 오른쪽을 수평면에 투영해 카메라 위치의 기준으로 삼는다.
	FVector CameraForwardDirection = GetActorForwardVector();
	CameraForwardDirection.Z = 0.0f;

	if (!CameraForwardDirection.Normalize())
	{
		CameraForwardDirection = FVector::ForwardVector;
	}

	FVector CameraRightDirection = GetActorRightVector();
	CameraRightDirection.Z = 0.0f;

	if (!CameraRightDirection.Normalize())
	{
		CameraRightDirection = FVector::RightVector;
	}

	// 카메라를 조준 지점과 같은 높이에 두어 위아래로 기울지 않은 구도를 만든다.
	const FVector CameraLocation =
		FocusLocation
		+ CameraForwardDirection * DialogueCameraDistance
		+ CameraRightDirection * DialogueCameraSideOffset;

	const FRotator CameraRotation = (FocusLocation - CameraLocation).Rotation();

	DialogueCamera->SetWorldLocation(CameraLocation);
	DialogueCamera->SetWorldRotation(CameraRotation);
	DialogueCamera->SetFieldOfView(DialogueCameraFOV);
}

FVector ATinoNPCCharacter::GetDialogueFocusLocation() const
{
	// 캐릭터 키나 캡슐 크기에 상관없이 얼굴을 잡도록 머리 본 위치를 사용한다.
	const USkeletalMeshComponent* HeadOwner = nullptr;

	if (IsValid(FaceMesh) && FaceMesh->DoesSocketExist(HeadBoneName))
	{
		HeadOwner = FaceMesh;
	}
	else if (IsValid(BodyMesh) && BodyMesh->DoesSocketExist(HeadBoneName))
	{
		HeadOwner = BodyMesh;
	}

	if (HeadOwner != nullptr)
	{
		return HeadOwner->GetSocketLocation(HeadBoneName) + FVector(0.0f, 0.0f, DialogueFocusHeightOffset);
	}

	return GetActorLocation() + FVector(0.0f, 0.0f, DialogueTargetHeight + DialogueFocusHeightOffset);
}

void ATinoNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	CacheAnimationMeshes();
}

UDialogueData* ATinoNPCCharacter::SelectDialogueData(const UQuestComponent* PlayerQuest) const
{
	// 퀘스트를 안 주는 NPC이거나 플레이어에게 퀘스트 컴포넌트가 없으면 기본 대사만 쓴다.
	if (!IsValid(QuestToGrant) || PlayerQuest == nullptr)
	{
		return DialogueData;
	}

	UDialogueData* Selected = nullptr;

	switch (PlayerQuest->GetQuestState(QuestToGrant))
	{
	case EQuestState::InProgress:
		Selected = InProgressDialogueData;
		break;
	case EQuestState::ReadyToComplete:
		Selected = ReadyToCompleteDialogueData;
		break;
	case EQuestState::Completed:
		Selected = CompletedDialogueData;
		break;
	default:
		Selected = DialogueData;
		break;
	}

	// 해당 상태의 대사를 지정하지 않았으면 기본 대사로 대체한다.
	if (IsValid(Selected))
	{
		return Selected;
	}

	return DialogueData.Get();
}

void ATinoNPCCharacter::CacheAnimationMeshes()
{
	// 메타휴먼 블루프린트는 Character 기본 메시를 비워두고 Body/Face를 따로 붙이는 구조라
	// GetMesh()로는 실제 메시를 얻을 수 없다.
	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	GetComponents<USkeletalMeshComponent>(SkeletalMeshes);

	for (USkeletalMeshComponent* Mesh : SkeletalMeshes)
	{
		if (!IsValid(Mesh))
		{
			continue;
		}

		if (Mesh->GetFName() == BodyMeshComponentName)
		{
			BodyMesh = Mesh;
		}
		else if (Mesh->GetFName() == FaceMeshComponentName)
		{
			FaceMesh = Mesh;
		}
	}

	// 일반 캐릭터처럼 메시가 하나뿐인 NPC는 Character 기본 메시를 몸으로 쓴다.
	if (!IsValid(BodyMesh))
	{
		BodyMesh = GetMesh();
	}

	if (TalkFaceMontage != nullptr && !IsValid(FaceMesh))
	{
		UE_LOG(LogTinoNPC, Warning,
			TEXT("%s: 표정 몽타주가 지정되어 있지만 '%s' 이름의 메시를 찾지 못했다."),
			*GetName(), *FaceMeshComponentName.ToString());
	}
}

void ATinoNPCCharacter::PlayTalkAnimation()
{
	PlayMontageOnMesh(BodyMesh, TalkBodyMontage);
	PlayMontageOnMesh(FaceMesh, TalkFaceMontage);
}

void ATinoNPCCharacter::StopTalkAnimation()
{
	StopMontageOnMesh(BodyMesh, TalkBodyMontage, TalkMontageBlendOutTime);
	StopMontageOnMesh(FaceMesh, TalkFaceMontage, TalkMontageBlendOutTime);
}

void ATinoNPCCharacter::PlayMontageOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage)
{
	if (!IsValid(Mesh) || !IsValid(Montage))
	{
		return;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();

	if (AnimInstance == nullptr)
	{
		return;
	}

	// 이미 같은 몽타주가 재생 중이면 처음부터 다시 재생해 대사마다 동작이 새로 시작되게 한다.
	AnimInstance->Montage_Play(Montage);
}

void ATinoNPCCharacter::StopMontageOnMesh(USkeletalMeshComponent* Mesh, UAnimMontage* Montage, float BlendOutTime)
{
	if (!IsValid(Mesh) || !IsValid(Montage))
	{
		return;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();

	if (AnimInstance == nullptr)
	{
		return;
	}

	AnimInstance->Montage_Stop(BlendOutTime, Montage);
}
