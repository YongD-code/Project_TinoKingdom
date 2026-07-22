#include "TinoCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/Constants/TinoCollision.h"
#include "Project_TinoKingdom/DataAsset/AttackComboData.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoCombat, Log, All);

UTinoCombatComponent::UTinoCombatComponent()
{
	// 공격 판정은 Anim Notify State 구간에서만 실행하므로 상시 Tick은 필요 없다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UTinoCombatComponent::InitializeCombat(USkeletalMeshComponent* InAnimationMesh)
{
	AnimationMesh = InAnimationMesh;
}

void UTinoCombatComponent::SetEquippedAttackData(UAttackComboData* InAttackData)
{
	// nullptr도 유효한 입력, 무기를 해제하면 nullptr이 전달되고 GetEffectiveAttackData()가 DefaultAttackData를 선택
	EquippedAttackData = InAttackData;
}

bool UTinoCombatComponent::RequestAttack()
{
	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (OwnerCharacter->bPressedJump || !MovementComponent->IsMovingOnGround())
	{
		return false;
	}

	UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: 공격을 재생할 AnimInstance가 없습니다."), *GetNameSafe(OwnerCharacter));
		return false;
	}

	if (!IsAttacking())
	{
		return StartComboAttack(AnimInstance, ResolveAttackDataForNewAttack());
	}

	return TryQueueNextCombo(AnimInstance);
}

bool UTinoCombatComponent::IsAttacking() const
{
	return ActiveAttackData != nullptr;
}

void UTinoCombatComponent::SetComboInputWindowOpen(bool bIsOpen)
{
	bComboInputWindowOpen = bIsOpen;

	if (bIsOpen)
	{
		// 콤보 입력창 하나에서는 다음 공격을 한 번만 예약할 수 있다.
		bComboInputConsumed = false;
	}
}

void UTinoCombatComponent::BeginAttackHitWindow()
{
	bAttackHitWindowOpen = false;
	ActiveAttackSectionIndex = INDEX_NONE;
	HitActorsThisWindow.Reset();

	// 공격 몽타주 외의 Notify가 잘못 전달된 경우에는 판정창을 열지 않는다.
	if (ActiveAttackData == nullptr)
	{
		return;
	}

	const int32 FoundSectionIndex = FindActiveComboAttackSectionIndex();
	if (!ActiveAttackData->ComboSection.IsValidIndex(FoundSectionIndex))
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: 현재 공격 섹션 데이터를 찾을 수 없습니다."), *GetNameSafe(OwnerCharacter));
		return;
	}

	ActiveAttackSectionIndex = FoundSectionIndex;
	bAttackHitWindowOpen = true;

	// Notify 구간이 짧더라도 최소 한 번은 판정한다.
	PerformAttackTrace();
}

void UTinoCombatComponent::TickAttackHitWindow()
{
	PerformAttackTrace();
}

void UTinoCombatComponent::EndAttackHitWindow()
{
	bAttackHitWindowOpen = false;
	ActiveAttackSectionIndex = INDEX_NONE;
	HitActorsThisWindow.Reset();
}

void UTinoCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 이 컴포넌트는 ACharacter 전용이다.
	OwnerCharacter = CastChecked<ACharacter>(GetOwner());

	// 일반 캐릭터는 GetMesh(), 별도 Driver Mesh를 쓰는 캐릭터는 InitializeCombat()으로 지정한다.
	if (AnimationMesh == nullptr)
	{
		AnimationMesh = OwnerCharacter->GetMesh();
	}

	checkf(AnimationMesh != nullptr, TEXT("%s: UTinoCombatComponent에 AnimationMesh가 지정되지 않았습니다."),
		*GetNameSafe(OwnerCharacter));
}

UAttackComboData* UTinoCombatComponent::ResolveAttackDataForNewAttack() const
{
	return EquippedAttackData != nullptr ? EquippedAttackData.Get() : DefaultAttackData.Get();
}

bool UTinoCombatComponent::StartComboAttack(UAnimInstance* AnimInstance, UAttackComboData* AttackData)
{
	// 외부에서 설정하는 DataAsset의 필수 구성만 공격 시작 경계에서 한 번 검증한다.
	if (AttackData == nullptr || AttackData->AttackMontage == nullptr || AttackData->ComboSection.IsEmpty())
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: 유효한 공격 데이터가 지정되지 않았습니다."), *GetNameSafe(OwnerCharacter));
		return false;
	}

	UAnimMontage* AttackMontage = AttackData->AttackMontage;
	const FName FirstSectionName = AttackData->ComboSection[0].SectionName;
	if (!AttackMontage->IsValidSectionName(FirstSectionName))
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: 몽타주 %s에 섹션 %s이 존재하지 않습니다."),
			*GetNameSafe(OwnerCharacter), *GetNameSafe(AttackMontage), *FirstSectionName.ToString());
		return false;
	}
	
	ActiveAttackData = AttackData;
	QueuedComboIndex = 0;
	bComboInputWindowOpen = false;
	bComboInputConsumed = false;

	if (AnimInstance->Montage_Play(AttackMontage) <= 0.f)
	{
		UE_LOG(LogTinoCombat, Error, TEXT("%s: 공격 몽타주 %s 재생에 실패했습니다."),
			*GetNameSafe(OwnerCharacter), *GetNameSafe(AttackMontage));
		ResetCombo();
		return false;
	}

	AnimInstance->Montage_JumpToSection(FirstSectionName, AttackMontage);

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &UTinoCombatComponent::OnAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AttackMontage);

	OwnerCharacter->ConsumeMovementInputVector();
	OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();

	return true;
}

bool UTinoCombatComponent::TryQueueNextCombo(UAnimInstance* AnimInstance)
{
	if (!bComboInputWindowOpen || bComboInputConsumed)
	{
		return false;
	}

	const int32 NextComboIndex = QueuedComboIndex + 1;
	if (!ActiveAttackData->ComboSection.IsValidIndex(NextComboIndex))
	{
		return false;
	}

	const FName CurrentSectionName = ActiveAttackData->ComboSection[QueuedComboIndex].SectionName;
	const FName NextSectionName = ActiveAttackData->ComboSection[NextComboIndex].SectionName;

	AnimInstance->Montage_SetNextSection(CurrentSectionName, NextSectionName, ActiveAttackData->AttackMontage);

	QueuedComboIndex = NextComboIndex;
	bComboInputConsumed = true;

	return true;
}

void UTinoCombatComponent::ResetCombo()
{
	QueuedComboIndex = INDEX_NONE;
	bComboInputWindowOpen = false;
	bComboInputConsumed = false;

	EndAttackHitWindow();
	ActiveAttackData = nullptr;
}

void UTinoCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 공격 상태가 이미 초기화됐다면 처리할 것이 없다.
	if (ActiveAttackData == nullptr)
	{
		return;
	}
	// 현재 공격 몽타주가 아닌 다른 몽타주의 종료 콜백은 무시한다.
	if (Montage != ActiveAttackData->AttackMontage)
	{
		return;
	}
	ResetCombo();
}

int32 UTinoCombatComponent::FindActiveComboAttackSectionIndex() const
{
	const UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance();
	const FName CurrentSectionName = AnimInstance->Montage_GetCurrentSection(ActiveAttackData->AttackMontage);

	return ActiveAttackData->ComboSection.IndexOfByPredicate(
		[CurrentSectionName](const FComboAttackSectionData& SectionData)
		{
			return SectionData.SectionName == CurrentSectionName;
		});
}

void UTinoCombatComponent::PerformAttackTrace()
{
	if (!bAttackHitWindowOpen)
	{
		return;
	}

	const FComboAttackSectionData& AttackSection =
		ActiveAttackData->ComboSection[ActiveAttackSectionIndex];

	// MaxHitTargets가 0이면 대상 수 제한이 없다.
	if (AttackSection.MaxHitTargets > 0 && HitActorsThisWindow.Num() >= AttackSection.MaxHitTargets)
	{
		return;
	}

	UWorld* World = GetWorld();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const FVector Up = OwnerCharacter->GetActorUpVector();
	const FVector Start = OwnerCharacter->GetActorLocation()
		+ Forward * AttackSection.TraceStartForwardOffset
		+ Up * AttackSection.TraceHeightOffset;
	const FVector End = Start + Forward * AttackSection.TraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TinoMeleeTrace), false, OwnerCharacter);
	TArray<FHitResult> HitResults;
	World->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity,
		TinoCollision::Action, FCollisionShape::MakeSphere(AttackSection.TraceRadius), QueryParams);

	HitResults.Sort(
		[](const FHitResult& A, const FHitResult& B)
		{
			return A.Distance < B.Distance;
		});

	const FColor DebugColor = HitResults.IsEmpty() ? FColor::Green : FColor::Red;
	DrawDebugSphere(World, Start, AttackSection.TraceRadius, 16, DebugColor, 
		false, 0.1f, 0, 1.f);
	DrawDebugSphere(World, End, AttackSection.TraceRadius, 16, DebugColor, 
		false, 0.1f, 0, 1.f);
	DrawDebugLine(World, Start, End, DebugColor, 
		false, 0.1f, 0, 1.f);

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor == nullptr)
		{
			continue;
		}

		const TObjectKey<AActor> HitActorKey(HitActor);
		if (HitActorsThisWindow.Contains(HitActorKey))
		{
			continue;
		}

		HitActorsThisWindow.Add(HitActorKey);

		UGameplayStatics::ApplyPointDamage(HitActor, AttackSection.Damage, Forward, HitResult,
			OwnerCharacter->GetController(), OwnerCharacter, UDamageType::StaticClass());

		UE_LOG(LogTinoCombat, Log, TEXT("%s 공격이 %s를 검출"), *GetNameSafe(OwnerCharacter), *GetNameSafe(HitActor));

		if (AttackSection.MaxHitTargets > 0 && HitActorsThisWindow.Num() >= AttackSection.MaxHitTargets)
		{
			break;
		}
	}
}
