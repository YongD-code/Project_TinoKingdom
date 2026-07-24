// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/ObjectKey.h"
#include "TinoCombatComponent.generated.h"

class AActor;
class ACharacter;
class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;
class UAttackComboData;

UCLASS( ClassGroup = (Tino), meta=(BlueprintSpawnableComponent) )
class PROJECT_TINOKINGDOM_API UTinoCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// 상시 Tick 사용 여부 등 컴포넌트의 기본값을 설정한다.
	UTinoCombatComponent();

	// 공격 몽타주를 실제로 재생할 Skeletal Mesh를 지정한다.
	void InitializeCombat(USkeletalMeshComponent* InAnimationMesh);

	// 장착 완료된 무기의 공격 데이터를 다음 공격용으로 저장한다.
	void SetEquippedAttackData(UAttackComboData* InAttackData);

	// 새 공격을 시작하거나 열린 입력창에서 다음 콤보를 예약한다.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool RequestAttack();

	// 현재 실행 중인 공격이 있는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttacking() const;

	// Anim Notify가 전달한 콤보 입력 허용 상태를 갱신한다.
	void SetComboInputWindowOpen(bool bIsOpen);

	// 공격 판정 구간을 열고 현재 공격을 즉시 한 번 판정한다.
	void BeginAttackHitWindow();

	// 공격 판정 Notify 구간에서 매 Tick마다 판정을 수행한다.
	void TickAttackHitWindow();

	// 공격 판정 구간과 해당 구간의 중복 타격 기록을 초기화한다.
	void EndAttackHitWindow();

protected:
	// 소유 Character와 공격 몽타주를 재생할 기본 Mesh를 캐싱한다.
	virtual void BeginPlay() override;

private:
	// 장착 공격 데이터가 있으면 반환하고, 없으면 기본 공격 데이터를 반환한다.
	UAttackComboData* ResolveAttackDataForNewAttack() const;

	// 선택한 공격 데이터를 고정하고 첫 콤보 몽타주를 재생한다.
	// 몽타주 재생에 성공하면 true를 반환한다.
	bool StartComboAttack(UAnimInstance* AnimInstance, UAttackComboData* AttackData);

	// 콤보 입력창이 열려 있으면 현재 공격 데이터의 다음 콤보를 예약한다.
	bool TryQueueNextCombo(UAnimInstance* AnimInstance);

	// 콤보, 공격 판정 구간, 현재 공격 데이터 상태를 초기화한다.
	void ResetCombo();

	// 공격 몽타주가 종료되거나 중단되면 현재 공격 상태를 초기화한다.
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 현재 몽타주 섹션과 일치하는 ComboSection 배열 인덱스를 찾는다.
	int32 FindActiveComboAttackSectionIndex() const;

	// 현재 공격 섹션의 범위를 Sweep하고 중복되지 않은 대상에 피해를 적용한다.
	void PerformAttackTrace();

private:
	// 장착 공격 데이터가 없을 때 사용할 캐릭터의 기본 공격 데이터.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAttackComboData> DefaultAttackData;

	// 실제 장착 완료된 무기가 다음 공격에 제공할 공격 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UAttackComboData> EquippedAttackData;

	// 현재 공격이 종료될 때까지 고정해서 사용하는 공격 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UAttackComboData> ActiveAttackData;

	// 위치, 이동, 컨트롤러에 접근하기 위한 소유 Character 캐시.
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	// 공격 몽타주를 실제로 재생하는 Skeletal Mesh.
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> AnimationMesh;

	// 현재 공격에서 마지막으로 예약된 ComboSection 인덱스.
	int32 QueuedComboIndex = INDEX_NONE;

	// 현재 콤보 입력을 받을 수 있는 Anim Notify 구간인지 나타낸다.
	bool bComboInputWindowOpen = false;

	// 하나의 입력창에서 다음 콤보 입력을 이미 사용했는지 나타낸다.
	bool bComboInputConsumed = false;

	// 현재 공격 판정에 사용하고 있는 ComboSection 인덱스.
	int32 ActiveAttackSectionIndex = INDEX_NONE;

	// 현재 공격 판정 Anim Notify 구간이 열려 있는지 나타낸다.
	bool bAttackHitWindowOpen = false;

	// 현재 판정 구간에서 이미 피해를 준 Actor의 식별 키 집합.
	// TObjectKey를 사용해 GC 이후에도 TSet의 해시 규칙을 유지한다.
	TSet<TObjectKey<AActor>> HitActorsThisWindow;
};
