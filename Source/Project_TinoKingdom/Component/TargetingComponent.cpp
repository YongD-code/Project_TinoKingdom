// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetingComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Project_TinoKingdom/Interface/TargetableInterface.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTargetingComponent::TryLockOnFromCrosshair()
{
	FVector ViewLocation;
	FRotator ViewRotation;
	
	OwnerCharacter->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
	
	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + ViewRotation.Vector() * MaxTargetingDistance;
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnTrace), false, OwnerCharacter);
	FHitResult HitResult;
	
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const FVector DebugEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
	
	DrawDebugLine(GetWorld(), TraceStart, DebugEnd, bHit ? FColor::Yellow : FColor::Red,
		false, 1.5f, 0, 1.5f);
	
	AActor* HitActor = HitResult.GetActor();
	if (HitActor == nullptr || !HitActor->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s는 타게팅 가능한 액터가 아님"), *GetNameSafe(HitActor));
		return;
	}
	if (!ITargetableInterface::Execute_CanBeTargeted(HitActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s는 현재 타게팅 불가능"), *GetNameSafe(HitActor));
		return;
	}
	
	if (CurrentTarget.Get() == HitActor)
	{
		ClearTarget();
		return;
	}
	SetTarget(HitActor);
	const FVector LockOnLocation = ITargetableInterface::Execute_GetLockOnLocation(HitActor);
	
	DrawDebugSphere(GetWorld(), LockOnLocation, 15.f, 12, FColor::Blue, false, 1.5f);
	
}

void UTargetingComponent::ClearTarget()
{
	AActor* PreviousTarget = CurrentTarget.Get();
	if (PreviousTarget == nullptr)
	{
		return;
	}
	CurrentTarget.Reset();
	OnTargetChanged.Broadcast(PreviousTarget, nullptr);
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	CameraBoom = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
}

void UTargetingComponent::SetTarget(AActor* NewTarget)
{
	AActor* PreviousTarget = CurrentTarget.Get();
	if (PreviousTarget == NewTarget)
	{
		return;
	}
	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(PreviousTarget, NewTarget);
}
