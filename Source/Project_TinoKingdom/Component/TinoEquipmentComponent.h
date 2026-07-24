// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TinoEquipmentComponent.generated.h"

class ACharacter;
class ATinoEquipmentActor;
class UEquipmentLoadoutData;
class USkeletalMeshComponent;
class UTinoCombatComponent;

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UTinoEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTinoEquipmentComponent();
	
	// 지정한 장비 조합을 생성하고 좌우 소켓에 부착
	bool EquipLoadout(UEquipmentLoadoutData* InLoadout);
	
	// 현재 장비 Actor를 제거하고 맨손 공격 데이터로 되돌림
	void Unequip();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 장비 Actor 하나를 생성하고 지정한 소켓에 부착
	ATinoEquipmentActor* SpawnAndAttachEquipment(TSubclassOf<ATinoEquipmentActor> EquipmentClass, FName SocketName);

	// 장비 Actor를 제거하고 보관 중인 참조를 비움
	void DestroyEquipmentActor(TObjectPtr<ATinoEquipmentActor>& InOutEquipmentActor);

	// 게임 시작 시 자동으로 장착할 기본 장비 조합.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquipmentLoadoutData> DefaultLoadout;

	// 오른손 장비를 붙일 소켓 이름.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Socket", meta = (AllowPrivateAccess = "true"))
	FName RightHandSocketName = FName(TEXT("RightHandEquipmentSocket"));

	// 왼손 장비를 붙일 소켓 이름.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Socket", meta = (AllowPrivateAccess = "true"))
	FName LeftHandSocketName = FName(TEXT("LeftHandEquipmentSocket"));

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> AttachmentMesh;

	UPROPERTY(Transient)
	TObjectPtr<UTinoCombatComponent> CombatComponent;

	// 런타임에 생성한 좌우 장비 Actor.
	UPROPERTY(Transient)
	TObjectPtr<ATinoEquipmentActor> RightHandEquipmentActor;

	UPROPERTY(Transient)
	TObjectPtr<ATinoEquipmentActor> LeftHandEquipmentActor;
};
