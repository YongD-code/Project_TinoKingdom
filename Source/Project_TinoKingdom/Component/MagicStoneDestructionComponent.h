#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MagicStoneDestructionComponent.generated.h"

class AEnemyCharacter;
class UGeometryCollection;
class UGeometryCollectionComponent;
class UStaticMesh;
class UStatComponent;

USTRUCT(BlueprintType)
struct FMagicStoneFractureMesh
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magic Stone")
	TSoftObjectPtr<UStaticMesh> IntactMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magic Stone")
	TSoftObjectPtr<UGeometryCollection> FracturedCollection;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMagicStoneEvent);

UCLASS(ClassGroup=(Tino), meta=(BlueprintSpawnableComponent))
class PROJECT_TINOKINGDOM_API UMagicStoneDestructionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMagicStoneDestructionComponent();

	float ReceiveStoneDamage(float DamageAmount, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Magic Stone")
	bool IsStoneBroken() const { return bBroken; }

	UFUNCTION(BlueprintPure, Category = "Magic Stone")
	bool IsStoneVulnerable() const { return bKingDefeated && !bBroken; }

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Magic Stone")
	TObjectPtr<AEnemyCharacter> KingEnemy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magic Stone", meta=(ClampMin="2", ClampMax="3"))
	int32 HitsToBreak = 3;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Magic Stone")
	int32 ReceivedHits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magic Stone")
	TArray<FMagicStoneFractureMesh> FractureMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magic Stone|Physics", meta=(ClampMin="1"))
	float BreakStrain = 1000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magic Stone|Physics", meta=(ClampMin="0"))
	float DebrisSpeed = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magic Stone|Physics", meta=(ClampMin="1"))
	float DebrisLifetime = 8.0f;

	UPROPERTY(BlueprintAssignable, Category = "Magic Stone")
	FMagicStoneEvent OnStoneUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Magic Stone")
	FMagicStoneEvent OnStoneBroken;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindKing();
	UFUNCTION()
	void HandleKingDeath();
	bool SpawnDebris();
	void ApplyBreakFields();
	void ScatterDebris();

	UPROPERTY(Transient)
	TObjectPtr<UStatComponent> KingStats;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGeometryCollectionComponent>> DebrisComponents;

	FTimerHandle BindTimer;
	FTimerHandle BreakTimer;
	FTimerHandle ScatterTimer;
	bool bKingDefeated = false;
	bool bBroken = false;
};
