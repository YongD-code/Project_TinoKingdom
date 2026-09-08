#include "MagicStoneDestructionComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Field/FieldSystemObjects.h"
#include "GameFramework/Controller.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "Project_TinoKingdom/Character/PlayerCharacter.h"
#include "Project_TinoKingdom/Character/TinoNPCCharacter.h"
#include "Project_TinoKingdom/Component/DialogueComponent.h"
#include "Project_TinoKingdom/Component/StatComponent.h"
#include "Project_TinoKingdom/World/BossIntroCinematicActor.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMagicStone, Log, All);

UMagicStoneDestructionComponent::UMagicStoneDestructionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	const FString MeshPath = TEXT("/Game/Fab/Magic_crystal/magic_crystal/StaticMeshes/");
	for (const FString& MeshName : {FString(TEXT("magic_crystal")), FString(TEXT("magic_crystal1"))})
	{
		FMagicStoneFractureMesh& Entry = FractureMeshes.AddDefaulted_GetRef();
		Entry.IntactMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath + MeshName + TEXT(".") + MeshName));
		const FString CollectionName = TEXT("GC_") + MeshName;
		Entry.FracturedCollection = TSoftObjectPtr<UGeometryCollection>(FSoftObjectPath(
			MeshPath + TEXT("magic_crystal1_magic_crystal1/") + CollectionName + TEXT(".") + CollectionName));
	}
}

void UMagicStoneDestructionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner()->IsA<ATinoNPCCharacter>())
	{
		UE_LOG(LogMagicStone, Error, TEXT("%s: MagicStoneDestruction requires a TinoNPCCharacter owner."), *GetOwner()->GetName());
		return;
	}
	// Bind after actor/component BeginPlay so the king's initial HP has been initialized.
	BindTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::BindKing);
	for (const FMagicStoneFractureMesh& Entry : FractureMeshes)
	{
		Entry.IntactMesh.LoadSynchronous();
		Entry.FracturedCollection.LoadSynchronous();
	}
}

void UMagicStoneDestructionComponent::BindKing()
{
	if (!IsValid(KingEnemy))
	{
		TArray<AActor*> Intros;
		UGameplayStatics::GetAllActorsOfClass(this, ABossIntroCinematicActor::StaticClass(), Intros);
		TSet<AEnemyCharacter*> Candidates;
		for (AActor* Actor : Intros)
		{
			AEnemyCharacter* Candidate = CastChecked<ABossIntroCinematicActor>(Actor)->GetBossEnemy();
			if (IsValid(Candidate))
			{
				Candidates.Add(Candidate);
			}
		}
		if (Candidates.Num() == 1)
		{
			KingEnemy = *Candidates.CreateConstIterator();
		}
	}
	if (!IsValid(KingEnemy))
	{
		UE_LOG(LogMagicStone, Error, TEXT("%s: Assign King Enemy on the placed stone. Stone remains invulnerable."), *GetOwner()->GetName());
		return;
	}
	KingStats = KingEnemy->FindComponentByClass<UStatComponent>();
	if (!IsValid(KingStats))
	{
		UE_LOG(LogMagicStone, Error, TEXT("%s: King Enemy has no StatComponent."), *GetOwner()->GetName());
		return;
	}
	KingStats->OnDead.AddUniqueDynamic(this, &ThisClass::HandleKingDeath);
	if (KingEnemy->IsDead() || KingStats->IsDead())
	{
		HandleKingDeath();
	}
}

void UMagicStoneDestructionComponent::HandleKingDeath()
{
	if (!bKingDefeated)
	{
		bKingDefeated = true;
		UE_LOG(LogMagicStone, Log, TEXT("%s: King defeated; stone unlocked."), *GetOwner()->GetName());
		OnStoneUnlocked.Broadcast();
	}
}

float UMagicStoneDestructionComponent::ReceiveStoneDamage(float DamageAmount, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsStoneVulnerable() || DamageAmount <= 0.0f || !FMath::IsFinite(DamageAmount))
	{
		return 0.0f;
	}
	const APlayerCharacter* Player = EventInstigator ? Cast<APlayerCharacter>(EventInstigator->GetPawn()) : nullptr;
	if (!Player)
	{
		Player = Cast<APlayerCharacter>(DamageCauser);
	}
	if (!Player)
	{
		return 0.0f;
	}

	const int32 RequiredHits = FMath::Clamp(HitsToBreak, 2, 3);
	if (ReceivedHits + 1 >= RequiredHits)
	{
		// Keep the intact stone if assets are missing, allowing configuration to be diagnosed.
		if (!SpawnDebris())
		{
			return 0.0f;
		}
		bBroken = true;
		ReceivedHits = RequiredHits;
		if (ATinoNPCCharacter* NPC = Cast<ATinoNPCCharacter>(GetOwner()))
		{
			NPC->StopTalkAnimation();
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (PC && PC->GetPawn())
				{
					if (UDialogueComponent* Dialogue = PC->GetPawn()->FindComponentByClass<UDialogueComponent>())
					{
						Dialogue->CancelDialogueWithNPC(NPC);
					}
				}
			}
		}
		GetOwner()->SetActorEnableCollision(false);
		GetOwner()->SetActorHiddenInGame(true);
		GetOwner()->SetLifeSpan(FMath::Max(DebrisLifetime, 1.0f) + 1.0f);
		GetWorld()->GetTimerManager().SetTimer(BreakTimer, this, &ThisClass::ApplyBreakFields, 0.1f, false);
		OnStoneBroken.Broadcast();
	}
	else
	{
		++ReceivedHits;
	}
	UE_LOG(LogMagicStone, Log, TEXT("%s: Hits %d/%d."), *GetOwner()->GetName(), ReceivedHits, RequiredHits);
	return DamageAmount;
}

bool UMagicStoneDestructionComponent::SpawnDebris()
{
	TArray<UStaticMeshComponent*> Meshes;
	GetOwner()->GetComponents(Meshes);
	TArray<TPair<UStaticMeshComponent*, UGeometryCollection*>> Replacements;
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!Mesh->IsVisible() || !Mesh->GetStaticMesh())
		{
			continue;
		}
		for (const FMagicStoneFractureMesh& Entry : FractureMeshes)
		{
			if (Entry.IntactMesh.LoadSynchronous() == Mesh->GetStaticMesh())
			{
				UGeometryCollection* Collection = Entry.FracturedCollection.LoadSynchronous();
				if (!Collection)
				{
					UE_LOG(LogMagicStone, Error, TEXT("Missing fractured collection for %s."), *Mesh->GetName());
					return false;
				}
				Replacements.Emplace(Mesh, Collection);
				break;
			}
		}
	}
	if (Replacements.IsEmpty())
	{
		UE_LOG(LogMagicStone, Error, TEXT("%s: No visible static mesh matches Fracture Meshes."), *GetOwner()->GetName());
		return false;
	}

	AActor* Debris = GetWorld()->SpawnActor<AActor>();
	if (!Debris)
	{
		return false;
	}
	for (const auto& Replacement : Replacements)
	{
		UGeometryCollectionComponent* Geometry = NewObject<UGeometryCollectionComponent>(Debris);
		Debris->AddInstanceComponent(Geometry);
		if (!Debris->GetRootComponent())
		{
			Debris->SetRootComponent(Geometry);
		}
		Geometry->SetRestCollection(Replacement.Value);
		Geometry->SetWorldTransform(Replacement.Key->GetComponentTransform());
		Geometry->SetCollisionObjectType(ECC_PhysicsBody);
		Geometry->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Geometry->SetCollisionResponseToAllChannels(ECR_Ignore);
		Geometry->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Geometry->SetEnableDamageFromCollision(false);
		Geometry->RegisterComponent();
		Geometry->SetSimulatePhysics(true);
		DebrisComponents.Add(Geometry);
	}
	Debris->SetLifeSpan(FMath::Max(DebrisLifetime, 1.0f));
	return true;
}

void UMagicStoneDestructionComponent::ApplyBreakFields()
{
	for (UGeometryCollectionComponent* Geometry : DebrisComponents)
	{
		if (!IsValid(Geometry))
		{
			continue;
		}
		URadialFalloff* Strain = NewObject<URadialFalloff>(this);
		Strain->SetRadialFalloff(FMath::Max(BreakStrain, 1.0f), 1.0f, 1.0f, 0.0f,
			FMath::Max(Geometry->Bounds.SphereRadius * 2.0f, 100.0f), Geometry->Bounds.Origin, Field_FallOff_None);
		Geometry->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_ExternalClusterStrain, nullptr, Strain);
		
		UE_LOG(
		LogMagicStone, Warning,
		TEXT("GC %s: Registered=%d, PhysicsState=%d, Proxy=%d, Loading=%d"),
		*Geometry->GetName(),
		Geometry->IsRegistered(),
		Geometry->IsPhysicsStateCreated(),
		Geometry->GetPhysicsProxy() != nullptr,
		Geometry->GetIsObjectLoading()
	);
	}
	// Velocity is applied after Chaos has processed the cluster-breaking command.
	GetWorld()->GetTimerManager().SetTimer(ScatterTimer, this, &ThisClass::ScatterDebris, 0.1f, false);
}

void UMagicStoneDestructionComponent::ScatterDebris()
{
	for (UGeometryCollectionComponent* Geometry : DebrisComponents)
	{
		if (IsValid(Geometry))
		{
			URadialVector* Velocity = NewObject<URadialVector>(this);
			Velocity->SetRadialVector(FMath::Max(DebrisSpeed, 0.0f), Geometry->Bounds.Origin);
			Geometry->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity, nullptr, Velocity);
		}
	}
}

void UMagicStoneDestructionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(KingStats))
	{
		KingStats->OnDead.RemoveDynamic(this, &ThisClass::HandleKingDeath);
	}
	GetWorld()->GetTimerManager().ClearTimer(BindTimer);
	GetWorld()->GetTimerManager().ClearTimer(BreakTimer);
	GetWorld()->GetTimerManager().ClearTimer(ScatterTimer);
	Super::EndPlay(EndPlayReason);
}
