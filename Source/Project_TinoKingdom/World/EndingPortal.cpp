#include "EndingPortal.h"

#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogEndingPortal, Log, All);

AEndingPortal::AEndingPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EntranceRadius = CreateDefaultSubobject<USphereComponent>(TEXT("EntranceRadius"));
	EntranceRadius->SetupAttachment(SceneRoot);
	EntranceRadius->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EntranceRadius->SetSphereRadius(EntranceRadiusSize);

	PortalEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalEffect"));
	PortalEffect->SetupAttachment(SceneRoot);
	// 엔딩 A가 끝나기 전까지는 자동으로 켜지지 않는다.
	PortalEffect->bAutoActivate = false;
}

void AEndingPortal::BeginPlay()
{
	Super::BeginPlay();

	EntranceRadius->SetSphereRadius(EntranceRadiusSize);

	if (IsValid(PortalNiagaraSystem))
	{
		PortalEffect->SetAsset(PortalNiagaraSystem);
	}

	// 시작할 때는 숨어 있다가 RevealPortal 로만 등장한다.
	SetActorHiddenInGame(true);
	PortalEffect->Deactivate();
}

void AEndingPortal::RevealPortal()
{
	if (bRevealed)
	{
		return;
	}
	bRevealed = true;

	SetActorHiddenInGame(false);
	PortalEffect->Activate(true);

	UE_LOG(LogEndingPortal, Log, TEXT("%s: 귀환 포탈이 열렸습니다."), *GetName());
}

bool AEndingPortal::TryEnter(APlayerCharacter* PlayerCharacter)
{
	if (!bRevealed || !IsValid(PlayerCharacter))
	{
		return false;
	}

	// 반경 밖에서 상호작용 키를 눌러도 반응하지 않는다.
	const float DistanceSquared =
		FVector::DistSquared(GetActorLocation(), PlayerCharacter->GetActorLocation());
	if (DistanceSquared > FMath::Square(EntranceRadiusSize))
	{
		return false;
	}

	UE_LOG(LogEndingPortal, Log, TEXT("%s: 지상으로 이동합니다."), *GetName());
	return PlayerCharacter->TravelToEndingSurface();
}
