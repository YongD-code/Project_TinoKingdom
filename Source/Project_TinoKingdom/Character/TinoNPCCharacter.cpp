// Fill out your copyright notice in the Description page of Project Settings.


#include "TinoNPCCharacter.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "ConvexVolume.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/Constants/TinoGameplayTags.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Component/QuestComponent.h"
#include "Project_TinoKingdom/DataAsset/DialogueData.h"
#include "Project_TinoKingdom/DataAsset/QuestData.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAbilitySystemComponent.h"
#include "Project_TinoKingdom/GameplayAbilitySystem/TinoAttributeSet.h"
#include "SceneView.h"
#include "TimerManager.h"
#include "Slate/SceneViewport.h"

DEFINE_LOG_CATEGORY_STATIC(LogTinoNPC, Log, All);

// Sets default values
ATinoNPCCharacter::ATinoNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("TinoCapsule"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	AbilitySystemComponent = CreateDefaultSubobject<UTinoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UTinoAttributeSet>(TEXT("AttributeSet"));
	
	DialogueCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DialogueCamera"));
	DialogueCamera->SetupAttachment(GetRootComponent());

	// 실제 위치는 대화 시작 시 FocusDialogueCamera에서 머리 본 기준으로 다시 잡는다.
	// 여기 값은 에디터에서 대략적인 위치를 보여주기 위한 것이다.
	DialogueCamera->SetRelativeLocation(FVector(DialogueCameraDistance, DialogueCameraSideOffset, 60.0f));
	DialogueCamera->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	DialogueCamera->SetFieldOfView(DialogueCameraFOV);
}

UAbilitySystemComponent* ATinoNPCCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float ATinoNPCCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageAmount <= 0.f || IsDead())
	{
		return 0.f;
	}
	
	const float DamageToApply = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (DamageToApply <= 0.f)
	{
		return 0.f;
	}
	
	const float AppliedDamage = ApplyDamageGameplayEffect(DamageToApply, EventInstigator, DamageCauser);
	if (AppliedDamage <= 0.f)
	{
		return 0.f;
	}

	if (IsDead())
	{
		HandleDeath();
	}
	else
	{
		AActor* DamageInstigator = ResolveDamageInstigator(EventInstigator, DamageCauser);
		SetCombatTarget(DamageInstigator);
		PlayHitReaction();
	}
	
	UE_LOG(
		LogTinoNPC,
		Log,
		TEXT(
			"NPC 피격: %s, Damage %.1f, Health %.1f / %.1f, DamageCauser %s"
		),
		*GetName(),
		AppliedDamage,
		AttributeSet->GetHealth(),
		AttributeSet->GetMaxHealth(),
		*GetNameSafe(DamageCauser)
	);
	
	return AppliedDamage;
}

bool ATinoNPCCharacter::CanBeTargeted_Implementation() const
{
	return !IsDead();
}

FVector ATinoNPCCharacter::GetLockOnLocation_Implementation() const
{
	return GetDialogueFocusLocation();
}

bool ATinoNPCCharacter::IsDead() const
{
	return AttributeSet == nullptr || AttributeSet->GetHealth() <= 0.f;
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

	InitialSpawnTransform = GetActorTransform();

	// 대화 NPC는 제자리에서 동작한다. 월드 파티션의 지면 충돌이 늦게 로드되어도
	// CharacterMovement 중력으로 맵 아래로 추락하지 않도록 이동을 비활성화한다.
	SetNPCMovementEnabled(false);
	
	CacheAnimationMeshes();
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	const UTinoAttributeSet* RegisteredAttributeSet =
		AbilitySystemComponent->GetSet<UTinoAttributeSet>();

	const bool bAttributeSetRegistered = ensureMsgf(
		RegisteredAttributeSet == AttributeSet,
		TEXT("TinoNPCCharacter의 AttributeSet이 ASC에 올바르게 등록되지 않았습니다.")
	);

	if (bAttributeSetRegistered && InitializeDefaultAttributes())
	{
		UE_LOG(
			LogTinoNPC,
			Log,
			TEXT("NPC GAS 초기화 완료: %s, Health %.1f / %.1f, Defense %.1f"),
			*GetName(),
			AttributeSet->GetHealth(),
			AttributeSet->GetMaxHealth(),
			AttributeSet->GetDefense()
		);
	}
}

void ATinoNPCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnCheckTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ATinoNPCCharacter::SetNPCMovementEnabled(bool bEnabled)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent == nullptr)
	{
		return;
	}

	MovementComponent->StopMovementImmediately();
	if (bEnabled)
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}
	else
	{
		MovementComponent->DisableMovement();
	}
}

UDialogueData* ATinoNPCCharacter::SelectDialogueData(const UQuestComponent* PlayerQuest) const
{
	// 퀘스트를 안 주는 NPC이거나 플레이어에게 퀘스트 컴포넌트가 없으면 기본 대사만 쓴다.
	if (!HasAnyConfiguredQuest() || PlayerQuest == nullptr)
	{
		return DialogueData;
	}

	// 기존 단일 퀘스트는 항상 체인의 첫 단계로 취급한다.
	if (IsValid(QuestToGrant))
	{
		switch (PlayerQuest->GetQuestState(QuestToGrant))
		{
		case EQuestState::NotStarted:
			return DialogueData;
		case EQuestState::InProgress:
			return IsValid(InProgressDialogueData) ? InProgressDialogueData.Get() : DialogueData.Get();
		case EQuestState::ReadyToComplete:
			return IsValid(ReadyToCompleteDialogueData) ? ReadyToCompleteDialogueData.Get() : DialogueData.Get();
		case EQuestState::Completed:
			// 첫 퀘스트를 끝냈으면 아래의 추가 단계 검색으로 이어진다.
			break;
		}
	}

	if (const FTinoNPCQuestStage* Stage = FindActiveAdditionalQuestStage(PlayerQuest))
	{
		UDialogueData* SelectedDialogue = nullptr;

		switch (PlayerQuest->GetQuestState(Stage->Quest))
		{
		case EQuestState::InProgress:
			SelectedDialogue = Stage->InProgressDialogueData;
			break;
		case EQuestState::ReadyToComplete:
			SelectedDialogue = Stage->ReadyToCompleteDialogueData;
			break;
		case EQuestState::NotStarted:
		default:
			SelectedDialogue = Stage->OfferDialogueData;
			break;
		}

		// 단계별 대사를 빠뜨려도 대화 자체가 막히지 않도록 제안 대사,
		// NPC 기본 대사 순서로 대체한다.
		if (IsValid(SelectedDialogue))
		{
			return SelectedDialogue;
		}
		if (IsValid(Stage->OfferDialogueData))
		{
			return Stage->OfferDialogueData;
		}
		return DialogueData;
	}

	// 첫 퀘스트와 모든 추가 퀘스트를 끝낸 뒤에만 최종 완료 대사를 사용한다.
	return IsValid(CompletedDialogueData) ? CompletedDialogueData.Get() : DialogueData.Get();
}

UQuestData* ATinoNPCCharacter::GetActiveQuest(const UQuestComponent* PlayerQuest) const
{
	// 진행 정보를 받을 수 없는 호출에서도 첫 퀘스트를 예측 가능하게 반환한다.
	if (PlayerQuest == nullptr)
	{
		if (IsValid(QuestToGrant))
		{
			return QuestToGrant;
		}

		for (const FTinoNPCQuestStage& Stage : AdditionalQuestStages)
		{
			if (IsValid(Stage.Quest))
			{
				return Stage.Quest;
			}
		}

		return nullptr;
	}

	if (IsValid(QuestToGrant) && PlayerQuest->GetQuestState(QuestToGrant) != EQuestState::Completed)
	{
		return QuestToGrant;
	}

	if (const FTinoNPCQuestStage* Stage = FindActiveAdditionalQuestStage(PlayerQuest))
	{
		return Stage->Quest;
	}

	return nullptr;
}

const FTinoNPCQuestStage* ATinoNPCCharacter::FindActiveAdditionalQuestStage(
	const UQuestComponent* PlayerQuest) const
{
	for (const FTinoNPCQuestStage& Stage : AdditionalQuestStages)
	{
		if (!IsValid(Stage.Quest))
		{
			continue;
		}

		if (PlayerQuest == nullptr || PlayerQuest->GetQuestState(Stage.Quest) != EQuestState::Completed)
		{
			return &Stage;
		}
	}

	return nullptr;
}

bool ATinoNPCCharacter::HasAnyConfiguredQuest() const
{
	if (IsValid(QuestToGrant))
	{
		return true;
	}

	for (const FTinoNPCQuestStage& Stage : AdditionalQuestStages)
	{
		if (IsValid(Stage.Quest))
		{
			return true;
		}
	}

	return false;
}

void ATinoNPCCharacter::CacheAnimationMeshes()
{
	// 메타휴먼 블루프린트는 Character 기본 메시를 비워두고 Body/Face를 따로 붙이는 구조라
	// GetMesh()로는 실제 메시를 얻을 수 없다.
	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	GetComponents<USkeletalMeshComponent>(SkeletalMeshes);

	for (USkeletalMeshComponent* CurrentMesh : SkeletalMeshes)
	{
		if (!IsValid(CurrentMesh))
		{
			continue;
		}

		if (CurrentMesh->GetFName() == BodyMeshComponentName)
		{
			BodyMesh = CurrentMesh;
		}
		else if (CurrentMesh->GetFName() == FaceMeshComponentName)
		{
			FaceMesh = CurrentMesh;
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

void ATinoNPCCharacter::OnDialogueCompleted_Implementation(APlayerCharacter* PlayerCharacter)
{
}

void ATinoNPCCharacter::PlayHitReaction()
{
	PlayMontageOnMesh(BodyMesh, HitBodyMontage);
}

void ATinoNPCCharacter::HandleDeath()
{
	CombatTarget.Reset();
	StopTalkAnimation();
	
	SetNPCMovementEnabled(false);
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	PlayMontageOnMesh(BodyMesh, DeathBodyMontage);
	
	StartRespawnCheck();
}

void ATinoNPCCharacter::StartRespawnCheck()
{
	OutOfViewStartTime = -1.0;
	
	const float CheckInterval = FMath::Max(RespawnVisibilityCheckInterval, 0.05);
	GetWorldTimerManager().SetTimer(RespawnCheckTimerHandle, this, &ATinoNPCCharacter::CheckRespawnCondition,
		CheckInterval, true);
}

void ATinoNPCCharacter::CheckRespawnCondition()
{
	if (!IsDead())
	{
		GetWorldTimerManager().ClearTimer(RespawnCheckTimerHandle);
		OutOfViewStartTime = -1.0;
		return;
	}
	
	if (IsInsidePlayerViewFrustum())
	{
		OutOfViewStartTime = -1.0;
		return;
	}
	
	const UWorld* World = GetWorld();
	const double CurrentTime = World->GetTimeSeconds();
	
	if (OutOfViewStartTime < 0.0)
	{
		OutOfViewStartTime = CurrentTime;
	}
	if (CurrentTime - OutOfViewStartTime < OutOfViewRespawnDelay)
	{
		return;
	}
	
	RespawnAtInitialTransform();
}

bool ATinoNPCCharacter::IsInsidePlayerViewFrustum() const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(PlayerController))
	{
		return true;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (LocalPlayer == nullptr || LocalPlayer->ViewportClient == nullptr)
	{
		return true;
	}

	FSceneViewport* SceneViewport = LocalPlayer->ViewportClient->GetGameViewport();
	if (SceneViewport == nullptr)
	{
		return true;
	}

	FSceneViewProjectionData ProjectionData;
	if (!LocalPlayer->GetProjectionData(SceneViewport, ProjectionData))
	{
		return true;
	}

	FConvexVolume ViewFrustum;
	GetViewFrustumBounds(ViewFrustum, ProjectionData.ComputeViewProjectionMatrix(), true);

	FVector BoundsOrigin;
	FVector BoundsExtent;
	GetActorBounds(false, BoundsOrigin, BoundsExtent);

	return ViewFrustum.IntersectBox(BoundsOrigin, BoundsExtent);
}

void ATinoNPCCharacter::RespawnAtInitialTransform()
{
	GetWorldTimerManager().ClearTimer(RespawnCheckTimerHandle);
	OutOfViewStartTime = -1.0;

	SetActorTransform(InitialSpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());

	StopMontageOnMesh(BodyMesh, DeathBodyMontage, 0.0f);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SetNPCMovementEnabled(false);

	UE_LOG(
		LogTinoNPC,
		Log,
		TEXT("NPC 부활: %s, Health %.1f / %.1f"),
		*GetName(),
		AttributeSet->GetHealth(),
		AttributeSet->GetMaxHealth()
	);
}

AActor* ATinoNPCCharacter::ResolveDamageInstigator(AController* EventInstigator, AActor* DamageCauser) const
{
	if (EventInstigator != nullptr)
	{
		if (APawn* InstigatorPawn = EventInstigator->GetPawn())
		{
			return InstigatorPawn;
		}
	}

	if (IsValid(DamageCauser))
	{
		if (APawn* CauserInstigator = DamageCauser->GetInstigator())
		{
			return CauserInstigator;
		}

		return DamageCauser;
	}

	return nullptr;
}

void ATinoNPCCharacter::SetCombatTarget(AActor* NewTarget)
{
	if (!IsValid(NewTarget) || NewTarget == this || IsDead())
	{
		return;
	}
	
	CombatTarget = NewTarget;
	UE_LOG(
		LogTinoNPC,
		Log,
		TEXT("%s의 전투 대상 설정: %s"),
		*GetName(),
		*GetNameSafe(NewTarget)
	);
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

	// 같은 몽타주가 재생 중이어도 다시 호출되면 처음부터 재생한다.
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

bool ATinoNPCCharacter::InitializeDefaultAttributes()
{
	if (!ensureMsgf(DefaultAttributesEffect != nullptr, TEXT("DefaultAttributesEffect가 지정되지 않음")))
	{
		return false;
	}
	
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	
	const FGameplayEffectSpecHandle EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		DefaultAttributesEffect, 1.f, EffectContext);
	if (!ensureMsgf(EffectSpecHandle.IsValid(), TEXT("NPC 기본 능력치 Gameplay Effect Spec 생성 실패")))
	{
		return false;
	}
	
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	
	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	
	return true;
}

float ATinoNPCCharacter::ApplyDamageGameplayEffect(float DamageAmount, AController* EventInstigator,
	AActor* DamageCauser)
{
	if (DamageAmount <= 0.f)
	{
		return 0.f;
	}
	if (!ensureMsgf(DamageEffect != nullptr, TEXT("DamageEffect 미지정")))
	{
		return 0.f;
	}
	
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	AActor* InstigatorActor = ResolveDamageInstigator(EventInstigator, DamageCauser);
	
	EffectContext.AddInstigator(InstigatorActor, DamageCauser);
	EffectContext.AddSourceObject(DamageCauser);
	
	FGameplayEffectSpecHandle EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		DamageEffect, 1.f, EffectContext);
	if (!ensureMsgf(EffectSpecHandle.IsValid(), TEXT("NPC 피해 Gameplay Effect Spec 생성 실패")))
	{
		return 0.f;
	}
	
	EffectSpecHandle.Data->SetSetByCallerMagnitude(TinoGameplayTags::Data_Damage, DamageAmount);
	
	const float PreviousHealth = AttributeSet->GetHealth();
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	
	return FMath::Max(PreviousHealth - AttributeSet->GetHealth(), 0.f);
}
