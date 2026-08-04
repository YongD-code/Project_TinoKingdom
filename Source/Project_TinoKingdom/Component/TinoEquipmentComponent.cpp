// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoEquipmentComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
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
	
	if (CurrentLoadout == InLoadout)
	{
		return true;
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

	CurrentLoadout = InLoadout;
	OnEquipmentChanged.Broadcast(CurrentLoadout.Get());
	
	return true;
}

TArray<UEquipmentLoadoutData*> UTinoEquipmentComponent::GetSelectableLoadouts() const
{
	TArray<UEquipmentLoadoutData*> Result;
	Result.Reserve(SelectableLoadouts.Num());
	
	for (const TObjectPtr<UEquipmentLoadoutData>& Loadout : SelectableLoadouts)
	{
		if (IsValid(Loadout.Get()))
		{
			Result.Add(Loadout.Get());
		}
	}
	return Result;
}

void UTinoEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = CastChecked<ACharacter>(GetOwner());
	AttachmentMesh = OwnerCharacter->GetMesh();

	if (!IsValid(DefaultLoadout.Get()))
	{
		UE_LOG(LogTinoEquipment, Error, TEXT("%s: DefaultLoadout이 지정되지 않았습니다."),
			*GetNameSafe(OwnerCharacter));
		return;
	}
	
	if (!EquipLoadout(DefaultLoadout.Get()))
	{
		UE_LOG(LogTinoEquipment, Error, TEXT("%s: 기본 로드아웃 %s 적용에 실패했습니다."),
			*GetNameSafe(OwnerCharacter), *GetNameSafe(DefaultLoadout.Get()));
	}
}

void UTinoEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyEquipmentActor(RightHandEquipmentActor);
	DestroyEquipmentActor(LeftHandEquipmentActor);

	CurrentLoadout = nullptr;

	if (IsValid(CombatComponent.Get()))
	{
		CombatComponent->SetEquippedAttackData(nullptr);
	}

	CombatComponent = nullptr;
	AttachmentMesh = nullptr;
	OwnerCharacter = nullptr;
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
