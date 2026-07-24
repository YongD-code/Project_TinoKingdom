// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoEquipmentComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Project_TinoKingdom/Component/TinoCombatComponent.h"
#include "Project_TinoKingdom/DataAsset/EquipmentLoadoutData.h"
#include "Project_TinoKingdom/Equipment/TinoEquipmentActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoEquipment, Log, All);

UTinoEquipmentComponent::UTinoEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UTinoEquipmentComponent::EquipLoadout(UEquipmentLoadoutData* InLoadout)
{
	if (InLoadout == nullptr)
	{
		UE_LOG(LogTinoEquipment, Warning, TEXT("%s: 장착할 EquipmentLoadoutData가 없습니다."),
			*GetNameSafe(GetOwner()));
		return false;
	}

	// 새 조합이 완성될 때까지 기존 장비를 유지한다.
	ATinoEquipmentActor* NewRightHandActor =
		SpawnAndAttachEquipment(InLoadout->RightHandEquipmentClass, RightHandSocketName);

	if (InLoadout->RightHandEquipmentClass != nullptr && NewRightHandActor == nullptr)
	{
		return false;
	}

	ATinoEquipmentActor* NewLeftHandActor =
		SpawnAndAttachEquipment(InLoadout->LeftHandEquipmentClass, LeftHandSocketName);

	if (InLoadout->LeftHandEquipmentClass != nullptr && NewLeftHandActor == nullptr)
	{
		if (NewRightHandActor != nullptr)
		{
			NewRightHandActor->Destroy();
		}
		return false;
	}

	DestroyEquipmentActor(RightHandEquipmentActor);
	DestroyEquipmentActor(LeftHandEquipmentActor);

	RightHandEquipmentActor = NewRightHandActor;
	LeftHandEquipmentActor = NewLeftHandActor;
	CombatComponent->SetEquippedAttackData(InLoadout->AttackData);

	return true;
}

void UTinoEquipmentComponent::Unequip()
{
	DestroyEquipmentActor(RightHandEquipmentActor);
	DestroyEquipmentActor(LeftHandEquipmentActor);

	// nullptr을 전달하면 CombatComponent가 맨손 공격 데이터를 사용한다.
	CombatComponent->SetEquippedAttackData(nullptr);
}

void UTinoEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = CastChecked<ACharacter>(GetOwner());
	AttachmentMesh = OwnerCharacter->GetMesh();
	CombatComponent = OwnerCharacter->FindComponentByClass<UTinoCombatComponent>();

	checkf(CombatComponent != nullptr,
		TEXT("%s: UTinoEquipmentComponent에는 UTinoCombatComponent가 필요합니다."),
		*GetNameSafe(OwnerCharacter));

	if (DefaultLoadout != nullptr && !EquipLoadout(DefaultLoadout))
	{
		UE_LOG(LogTinoEquipment, Error, TEXT("%s: 기본 장비 조합 %s 장착에 실패했습니다."),
			*GetNameSafe(OwnerCharacter), *GetNameSafe(DefaultLoadout));
	}
}

void UTinoEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컴포넌트가 생성한 장비 Actor는 직접 정리한다.
	Unequip();
	Super::EndPlay(EndPlayReason);
}

ATinoEquipmentActor* UTinoEquipmentComponent::SpawnAndAttachEquipment(TSubclassOf<ATinoEquipmentActor> EquipmentClass,
	FName SocketName)
{
	// 장비 클래스 None은 해당 손을 비워 두겠다는 의미
	if (EquipmentClass == nullptr)
	{
		return nullptr;
	}

	if (!AttachmentMesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogTinoEquipment, Error, TEXT("%s: 장비 소켓 %s을 찾을 수 없습니다."),
			*GetNameSafe(OwnerCharacter), *SocketName.ToString());
		return nullptr;
	}
	
	// SpawnActor()로 Actor를 생성할 때 사용할 추가 설정들을 묶어놓은 구조체
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerCharacter;
	SpawnParameters.Instigator = OwnerCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FTransform SpawnTransform = AttachmentMesh->GetSocketTransform(SocketName, RTS_World);
	// 에셋에서 정한 장비 크기를 유지하도록 소켓 Scale은 적용하지 않는다.
	SpawnTransform.SetScale3D(FVector::OneVector);

	ATinoEquipmentActor* SpawnedEquipment = GetWorld()->SpawnActor<ATinoEquipmentActor>(
		EquipmentClass.Get(), SpawnTransform, SpawnParameters);

	if (!IsValid(SpawnedEquipment))
	{
		UE_LOG(LogTinoEquipment, Error, TEXT("%s: 장비 Actor %s 생성에 실패했습니다."),
			*GetNameSafe(OwnerCharacter), *GetNameSafe(EquipmentClass.Get()));
		return nullptr;
	}

	const bool bAttached = SpawnedEquipment->AttachToComponent(
		AttachmentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

	if (!bAttached)
	{
		UE_LOG(LogTinoEquipment, Error, TEXT("%s: 장비 Actor %s을 소켓 %s에 부착하지 못했습니다."),
			*GetNameSafe(OwnerCharacter), *GetNameSafe(SpawnedEquipment), *SocketName.ToString());
		SpawnedEquipment->Destroy();
		return nullptr;
	}

	return SpawnedEquipment;
}

void UTinoEquipmentComponent::DestroyEquipmentActor(TObjectPtr<ATinoEquipmentActor>& InOutEquipmentActor)
{
	if (IsValid(InOutEquipmentActor))
	{
		InOutEquipmentActor->Destroy();
	}

	InOutEquipmentActor = nullptr;
}
