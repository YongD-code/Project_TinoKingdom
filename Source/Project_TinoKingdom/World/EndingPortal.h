#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndingPortal.generated.h"

class APlayerCharacter;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USphereComponent;

// 마력석이 부서진 자리에 나타나는 귀환 포탈.
// 엔딩 시네마틱 A가 끝난 뒤에만 모습을 드러내고, 상호작용하면 지상으로 이동한다.
UCLASS()
class PROJECT_TINOKINGDOM_API AEndingPortal : public AActor
{
	GENERATED_BODY()

public:
	AEndingPortal();

	// 엔딩 시네마틱 A가 끝나는 시점에 호출한다. 이전까지는 보이지도, 반응하지도 않는다.
	UFUNCTION(BlueprintCallable, Category = "Ending Portal")
	void RevealPortal();

	UFUNCTION(BlueprintPure, Category = "Ending Portal")
	bool IsRevealed() const { return bRevealed; }

	// 플레이어가 반경 안에 있으면 지상 이동을 시작한다. 성공하면 true.
	bool TryEnter(APlayerCharacter* PlayerCharacter);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> EntranceRadius;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> PortalEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending Portal", meta = (ClampMin = "0.0"))
	float EntranceRadiusSize = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending Portal|Effect")
	TObjectPtr<UNiagaraSystem> PortalNiagaraSystem;

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ending Portal",
		meta = (AllowPrivateAccess = "true"))
	bool bRevealed = false;
};
