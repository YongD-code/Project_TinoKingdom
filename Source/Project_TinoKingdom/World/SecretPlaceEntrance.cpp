#include "SecretPlaceEntrance.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceSpline.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Component/InventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSecretPlaceEntrance, Log, All);

ASecretPlaceEntrance::ASecretPlaceEntrance()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EntranceRadius = CreateDefaultSubobject<USphereComponent>(TEXT("EntranceRadius"));
	EntranceRadius->SetupAttachment(SceneRoot);
	EntranceRadius->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EntranceRadius->SetCollisionResponseToAllChannels(ECR_Ignore);
	EntranceRadius->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EntranceRadius->SetGenerateOverlapEvents(true);

	BeamSpline = CreateDefaultSubobject<USplineComponent>(TEXT("BeamSpline"));
	BeamSpline->SetupAttachment(SceneRoot);
	BeamSpline->SetClosedLoop(false);

	EntranceEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EntranceEffect"));
	EntranceEffect->SetupAttachment(SceneRoot);
	EntranceEffect->SetAutoActivate(false);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultEntranceEffect(
		TEXT("/Game/SplineEffect2/Niagara/NS_SecretPlaceBeam.NS_SecretPlaceBeam"));
	if (DefaultEntranceEffect.Succeeded())
	{
		EntranceNiagaraSystem = DefaultEntranceEffect.Object;
		EntranceEffect->SetAsset(EntranceNiagaraSystem);
	}
}

void ASecretPlaceEntrance::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EntranceRadius->SetSphereRadius(EntranceRadiusSize);
	UpdateSplineShape();

	if (EntranceEffect->GetAsset() != EntranceNiagaraSystem)
	{
		EntranceEffect->SetAsset(EntranceNiagaraSystem);
	}
	ConfigureNiagaraSplineSource();
}

void ASecretPlaceEntrance::BeginPlay()
{
	Super::BeginPlay();

	EntranceRadius->SetSphereRadius(EntranceRadiusSize);
	UpdateSplineShape();
	ConfigureNiagaraSplineSource();
	EntranceEffect->DeactivateImmediate();
}

bool ASecretPlaceEntrance::TryUseItem(APlayerCharacter* PlayerCharacter, const FName ItemId)
{
	if (!IsValid(PlayerCharacter) || ItemId.IsNone())
	{
		return false;
	}

	UInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent();
	if (!IsValid(InventoryComponent))
	{
		return false;
	}

	const FInventoryItemStack* FoundItem = InventoryComponent->GetItems().FindByPredicate(
		[ItemId](const FInventoryItemStack& ItemStack)
		{
			return ItemStack.ItemId == ItemId && ItemStack.Count > 0;
		});
	if (FoundItem == nullptr)
	{
		return false;
	}

	const FInventoryItemStack ConsumedItem = *FoundItem;

	if (ItemId == MapItemId)
	{
		if (bRevealed || !InventoryComponent->RemoveItem(ItemId, 1))
		{
			return false;
		}

		if (!RevealEntrance())
		{
			RestoreConsumedItem(InventoryComponent, ConsumedItem);
			return false;
		}

		UE_LOG(LogSecretPlaceEntrance, Log, TEXT("%s 사용: 비밀 장소 입구가 공개되었습니다."), *ItemId.ToString());
		return true;
	}

	if (ItemId != RequiredItemId)
	{
		return false;
	}

	if (!bRevealed || !IsPlayerInside(PlayerCharacter))
	{
		UE_LOG(LogSecretPlaceEntrance, Log,
			TEXT("%s 사용 실패: 입구가 공개되지 않았거나 플레이어가 입장 범위 밖에 있습니다."),
			*ItemId.ToString());
		return false;
	}

	// 이동 상태에 소모 결과가 포함되도록 반드시 아이템을 먼저 제거한다.
	if (!InventoryComponent->RemoveItem(ItemId, 1))
	{
		return false;
	}

	if (!PlayerCharacter->TryOpenSecretPlace())
	{
		RestoreConsumedItem(InventoryComponent, ConsumedItem);
		return false;
	}

	return true;
}

bool ASecretPlaceEntrance::RevealEntrance()
{
	if (bRevealed)
	{
		return false;
	}

	if (!IsValid(EntranceEffect) || !IsValid(EntranceEffect->GetAsset()))
	{
		UE_LOG(LogSecretPlaceEntrance, Error,
			TEXT("비밀 장소 입구 공개 실패: Entrance Niagara System이 지정되지 않았습니다."));
		return false;
	}

	bRevealed = true;
	ConfigureNiagaraSplineSource();
	EntranceEffect->Activate(true);
	return true;
}

void ASecretPlaceEntrance::UpdateSplineShape()
{
	if (!IsValid(BeamSpline) || !IsValid(EntranceEffect))
	{
		return;
	}

	BeamSpline->ClearSplinePoints(false);
	BeamSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
	BeamSpline->AddSplinePoint(FVector(0.f, 0.f, BeamHeight), ESplineCoordinateSpace::Local, false);
	BeamSpline->SetSplinePointType(0, ESplinePointType::Linear, false);
	BeamSpline->SetSplinePointType(1, ESplinePointType::Linear, false);
	BeamSpline->UpdateSpline();

	// GPU Niagara가 긴 기둥을 화면 밖으로 잘못 컬링하지 않도록 로컬 고정 Bounds를 준다.
	const float HorizontalExtent = FMath::Max(EntranceRadiusSize, 500.f);
	EntranceEffect->SetSystemFixedBounds(FBox(
		FVector(-HorizontalExtent, -HorizontalExtent, -500.f),
		FVector(HorizontalExtent, HorizontalExtent, BeamHeight + 500.f)));
}

void ASecretPlaceEntrance::ConfigureNiagaraSplineSource()
{
	if (!IsValid(EntranceEffect) || !IsValid(EntranceEffect->GetAsset()) || SplineUserParameterName.IsNone())
	{
		return;
	}

	UNiagaraDataInterfaceSpline* SplineDataInterface =
		UNiagaraFunctionLibrary::GetDataInterface<UNiagaraDataInterfaceSpline>(
			EntranceEffect, SplineUserParameterName);
	if (SplineDataInterface == nullptr)
	{
		UE_LOG(LogSecretPlaceEntrance, Warning,
			TEXT("Niagara System %s에서 Spline 사용자 파라미터 %s를 찾지 못했습니다."),
			*GetNameSafe(EntranceEffect->GetAsset()), *SplineUserParameterName.ToString());
		return;
	}

	// Niagara Spline Data Interface는 Source Actor에서 첫 번째 SplineComponent를 찾는다.
	SplineDataInterface->SourceMode = ENDISpline_SourceMode::Default;
	SplineDataInterface->SoftSourceActor = this;
}

bool ASecretPlaceEntrance::IsPlayerInside(const APlayerCharacter* PlayerCharacter) const
{
	return IsValid(EntranceRadius) && IsValid(PlayerCharacter)
		&& EntranceRadius->IsOverlappingActor(PlayerCharacter);
}

void ASecretPlaceEntrance::RestoreConsumedItem(UInventoryComponent* InventoryComponent,
	const FInventoryItemStack& ItemStack)
{
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	InventoryComponent->AddItem(
		ItemStack.ItemId,
		ItemStack.DisplayName,
		1,
		ItemStack.Icon,
		ItemStack.ItemType,
		ItemStack.CookingTag,
		ItemStack.FoodEffectType,
		ItemStack.CookingPower,
		ItemStack.FoodResultData);
}
