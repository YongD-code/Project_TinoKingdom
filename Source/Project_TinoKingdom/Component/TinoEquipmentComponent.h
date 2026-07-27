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

// 나중에 UI, 캐릭터, 사운드 시스템 등등 다양한 객체에 이벤트를 보내게 Multicast로
// 델리게이트 : 실행할 함수를 미리 등록해 두었다가 특정 시점에 호출하는 Unreal의 콜백 시스템
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTinoEquipmentChanged, UEquipmentLoadoutData*, NewLoadout);

UCLASS( ClassGroup=(Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UTinoEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTinoEquipmentComponent();
	
	// 지정한 장비 조합을 생성하고 좌우 소켓에 부착
	// 이게 나중에 UI 등에서 사용하기 편할듯
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipLoadout(UEquipmentLoadoutData* InLoadout);
	
	// 현재 장비 Actor를 제거하고 맨손 공격 데이터로 되돌림
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void Unequip();
	
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsEquipped() const { return CurrentLoadout != nullptr; }
	
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UEquipmentLoadoutData* GetCurrentLoadout() const { return CurrentLoadout.Get(); }
	
	// 장비 장착 또는 해제 시 UI 등의 구독자에게 변경 결과를 알리는 이벤트
	// 블루프린트가 C++ 함수를 호출하는 것은 아니고 델리게이트에 이벤트를 등록할 수 있게
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnTinoEquipmentChanged OnEquipmentChanged;
	
	// 장비 선택 UI에 표시할 수 있는 Loadout 목록을 반환
	UFUNCTION(BlueprintPure, Category = "Equipment|Selection")
	TArray<UEquipmentLoadoutData*> GetSelectableLoadouts() const;
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

	// 장비 선택 UI에 표시할 Loadout 목록
	// 원본 참조를 장기간 멤버 변수로 저장할 용도이기 때문에 TObjectPtr로 보관
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Selection", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UEquipmentLoadoutData>> SelectableLoadouts;
	
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

	// nullptr이면 맨손
	UPROPERTY(Transient)
	TObjectPtr<UEquipmentLoadoutData> CurrentLoadout;
};
