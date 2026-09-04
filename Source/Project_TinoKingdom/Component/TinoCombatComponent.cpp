#include "TinoCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/Constants/TinoCollision.h"
#include "Project_TinoKingdom/DataAsset/AttackComboData.h"
#include "Project_TinoKingdom/Equipment/TinoEquipmentActor.h"
#include "Project_TinoKingdom/TinoRuntimeDebugDraw.h"
#include "Project_TinoKingdom/Component/TinoStateComponent.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAttributeSet.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoCombat, Log, All);

namespace
{
	const FName RightHandTraceBaseSocketName = FName("RightHandTraceBase");
	const FName RightHandTraceTipSocketName = FName("RightHandTraceTip");
	const FName LeftHandTraceBaseSocketName = FName("LeftHandTraceBase");
	const FName LeftHandTraceTipSocketName = FName("LeftHandTraceTip");
	const FName RightFootTraceBaseSocketName = FName("RightFootTraceBase");
	const FName RightFootTraceTipSocketName = FName("RightFootTraceTip");
}
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

void UTinoCombatComponent::SetEquipmentWeaponActors(ATinoEquipmentActor* InRightHandWeapon,
	ATinoEquipmentActor* InLeftHandWeapon)
{
	RightHandWeapon = InRightHandWeapon;
	LeftHandWeapon = InLeftHandWeapon;
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
	const FComboAttackSectionData& AttackSection = ActiveAttackData->ComboSection[ActiveAttackSectionIndex];
	GetAttackTracePoints(AttackSection.AttackSource, PreviousTraceBaseLocation, PreviousTraceTipLocation);
	
	// 최초로 위치 저장이 끝난 후 판정창을 연다.
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
	
	PreviousTraceBaseLocation = FVector::ZeroVector;
	PreviousTraceTipLocation = FVector::ZeroVector;
}

void UTinoCombatComponent::CancelAttack()
{
	if (!IsAttacking())
	{
		return;
	}
	// ResetCombo 이후 ActiveAttackData가 nullptr이 되므로 중단할 몽타주를 먼저 저장한다.
	UAnimMontage* AttackMontage = ActiveAttackData->AttackMontage;
	ResetCombo();
	
	UAnimInstance* AnimInstance = AnimationMesh->GetAnimInstance();
	if (AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		const float CancelBlendOutTime = 0.05f;
		AnimInstance->Montage_Stop(CancelBlendOutTime, AttackMontage);
	}
}

void UTinoCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 이 컴포넌트는 ACharacter 전용이다.
	OwnerCharacter = CastChecked<ACharacter>(GetOwner());
	StateComponent = OwnerCharacter->FindComponentByClass<UTinoStateComponent>();
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
	StateComponent->AddStateTag(TinoGameplayTags::State_Action_Attacking);
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
	const bool bWasAttacking = ActiveAttackData != nullptr;
	QueuedComboIndex = INDEX_NONE;
	bComboInputWindowOpen = false;
	bComboInputConsumed = false;

	EndAttackHitWindow();
	ActiveAttackData = nullptr;
	
	if (bWasAttacking)
	{
		StateComponent->RemoveStateTag(TinoGameplayTags::State_Action_Attacking);
	}
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

	const FComboAttackSectionData& AttackSection = ActiveAttackData->ComboSection[ActiveAttackSectionIndex];
	const float FinalDamage = CalculateAttackDamage(AttackSection.Damage);
	
	// MaxHitTargets가 0이면 대상 수 제한이 없다.
	if (AttackSection.MaxHitTargets > 0 && HitActorsThisWindow.Num() >= AttackSection.MaxHitTargets)
	{
		return;
	}
	
	FVector CurrentTraceBaseLocation = FVector::ZeroVector;
	FVector CurrentTraceTipLocation = FVector::ZeroVector;
	GetAttackTracePoints(AttackSection.AttackSource, CurrentTraceBaseLocation, CurrentTraceTipLocation);

	UWorld* World = GetWorld();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TinoMeleeTrace), false, OwnerCharacter);
	TArray<FHitResult> HitResults;
	constexpr int32 TraceCount = 5;
	
	for (int32 SampleIndex = 0; SampleIndex < TraceCount; ++SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(TraceCount - 1);
		const FVector SweepStart = FMath::Lerp(PreviousTraceBaseLocation, PreviousTraceTipLocation, Alpha);
		const FVector SweepEnd = FMath::Lerp(CurrentTraceBaseLocation, CurrentTraceTipLocation, Alpha);
		
		// 충돌 결과들을 모음, HitResults에 같은 Actor들이 들어갈 수 있음 FHitResult는 여러 충돌 정보를 갖고 있기 때문
		TArray<FHitResult> SampleHitResults;
		World->SweepMultiByChannel(SampleHitResults, SweepStart, SweepEnd, FQuat::Identity,
			TinoCollision::Action, FCollisionShape::MakeSphere(AttackSection.TraceRadius), QueryParams);
		HitResults.Append(SampleHitResults);

		/*const FColor DebugColor = SampleHitResults.IsEmpty() ? FColor::Green : FColor::Red;
		TinoRuntimeDebugDraw::DrawSweptSphere(
			World,
			SweepStart,
			SweepEnd,
			AttackSection.TraceRadius,
			DebugColor,
			0.75f
		);*/
	}
	
	PreviousTraceBaseLocation = CurrentTraceBaseLocation;
	PreviousTraceTipLocation = CurrentTraceTipLocation;
	
	HitResults.Sort(
		[](const FHitResult& A, const FHitResult& B)
		{
			return A.Time < B.Time;
		}
	);
	
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor == nullptr)
		{
			continue;
		}
		
		const TObjectKey<AActor> HitActorKey(HitActor);
		// 하나의 Hit Window에서는 한 번만 피해를 입히게
		if (HitActorsThisWindow.Contains(HitActorKey))
		{
			continue;
		}
		// 이번 Hit Window에 처리한 Actor로 등록
		HitActorsThisWindow.Add(HitActorKey);
		
		// 당장은 크게 필요없음. 나중에 적도 공격방향별로 피격 애니메이션을 다르게 할 때 사용
		FVector HitDirection = (HitResult.TraceEnd - HitResult.TraceStart).GetSafeNormal();
		if (HitDirection.IsNearlyZero())
		{
			HitDirection = Forward;
		}
		UGameplayStatics::ApplyPointDamage(HitActor, FinalDamage, HitDirection, HitResult, 
			OwnerCharacter->GetController(), OwnerCharacter, UDamageType::StaticClass());
		
		if (AttackSection.MaxHitTargets > 0 && HitActorsThisWindow.Num() >= AttackSection.MaxHitTargets)
		{
			break;
		}
	}
}

void UTinoCombatComponent::GetAttackTracePoints(EAttackSource AttackSource, FVector& OutBaseLocation,
	FVector& OutTipLocation) const
{
	switch (AttackSource)
	{
	case EAttackSource::RightWeapon:
		{
			RightHandWeapon->GetWeaponTracePoints(OutBaseLocation, OutTipLocation);
			return;
		}
	case EAttackSource::LeftWeapon:
		{
			LeftHandWeapon->GetWeaponTracePoints(OutBaseLocation, OutTipLocation);
			return;
		}
	case EAttackSource::RightHand:
		{
			checkf(AnimationMesh->DoesSocketExist(RightHandTraceBaseSocketName), TEXT("오른손 Base소켓이 없습니다."));
			checkf(AnimationMesh->DoesSocketExist(RightHandTraceTipSocketName), TEXT("오른손 Tip 소켓이 없습니다."));
			OutBaseLocation = AnimationMesh->GetSocketLocation(RightHandTraceBaseSocketName);
			OutTipLocation = AnimationMesh->GetSocketLocation(RightHandTraceTipSocketName);
			return;
		}
	case EAttackSource::LeftHand:
		{
			checkf(AnimationMesh->DoesSocketExist(LeftHandTraceBaseSocketName), TEXT("왼손 Base소켓이 없습니다."));
			checkf(AnimationMesh->DoesSocketExist(LeftHandTraceTipSocketName), TEXT("왼손 Tip 소켓이 없습니다."));
			OutBaseLocation = AnimationMesh->GetSocketLocation(LeftHandTraceBaseSocketName);
			OutTipLocation = AnimationMesh->GetSocketLocation(LeftHandTraceTipSocketName);
			return;
		}
	case EAttackSource::RightFoot:
		{
			checkf(AnimationMesh->DoesSocketExist(RightFootTraceBaseSocketName), TEXT("오른발 Base소켓이 없습니다."));
			checkf(AnimationMesh->DoesSocketExist(RightFootTraceTipSocketName), TEXT("오른발 Tip 소켓이 없습니다."));
			OutBaseLocation = AnimationMesh->GetSocketLocation(RightFootTraceBaseSocketName);
			OutTipLocation = AnimationMesh->GetSocketLocation(RightFootTraceTipSocketName);
			return;
		}
	case EAttackSource::LeftFoot:
	case EAttackSource::None:
	default:
		{
			UE_LOG(LogTinoCombat, Error, TEXT("Attack Source가 지정되지 않았습니다."));
			return;
		}
	}
}

float UTinoCombatComponent::CalculateAttackDamage(float ComboDamage) const
{
	const float SafeComboDamage = FMath::Max(ComboDamage, 1.f);
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(OwnerCharacter);
	if (AbilitySystemInterface == nullptr)
	{
		return SafeComboDamage;
	}
	
	const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
	const UTinoAttributeSet* AttributeSet = AbilitySystemComponent->GetSet<UTinoAttributeSet>();
	
	const float AttackPower = FMath::Max(AttributeSet->GetAttackPower(), 1.f);
	
	return AttackPower + SafeComboDamage;
		
}
