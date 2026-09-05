#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;

UCLASS()
class PROJECT_TINOKINGDOM_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Health")
	void SetHealthPercent(float HealthPercent);

protected:
	virtual void NativeConstruct() override;

private:
	void EnsureHealthBarWidget();
	FLinearColor GetHealthColor(float HealthPercent) const;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthProgressBar;
};
