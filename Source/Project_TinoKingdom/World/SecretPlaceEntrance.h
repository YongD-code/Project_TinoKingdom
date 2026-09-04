#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SecretPlaceEntrance.generated.h"

class APlayerCharacter;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USphereComponent;
class USplineComponent;
struct FInventoryItemStack;

UCLASS()
class PROJECT_TINOKINGDOM_API ASecretPlaceEntrance : public AActor
{
	GENERATED_BODY()

public:
	ASecretPlaceEntrance();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Secret Place")
	bool TryUseItem(APlayerCharacter* PlayerCharacter, FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "Secret Place")
	bool RevealEntrance();

	UFUNCTION(BlueprintPure, Category = "Secret Place")
	bool IsRevealed() const { return bRevealed; }

protected:
	virtual void BeginPlay() override;

private:
	void UpdateSplineShape();
	void ConfigureNiagaraSplineSource();
	bool IsPlayerInside(const APlayerCharacter* PlayerCharacter) const;
	static void RestoreConsumedItem(class UInventoryComponent* InventoryComponent,
		const FInventoryItemStack& ItemStack);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> EntranceRadius;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> BeamSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> EntranceEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret Place|Items")
	FName MapItemId = TEXT("SecretPlaceMap");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret Place|Items")
	FName RequiredItemId = TEXT("SecretPlaceKey");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret Place|Interaction")
	float EntranceRadiusSize = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret Place|Effect")
	float BeamHeight = 50000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret Place|Effect")
	TObjectPtr<UNiagaraSystem> EntranceNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret Place|Effect")
	FName SplineUserParameterName = TEXT("User.Spline");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Secret Place")
	bool bRevealed = false;
};
