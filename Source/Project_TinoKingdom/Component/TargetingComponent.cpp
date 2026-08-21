// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetingComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
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

	const FVector ViewDirection = ViewRotation.Vector();
	const FVector TraceEnd = ViewLocation + ViewDirection * MaxTargetingDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnTrace), false, OwnerCharacter);
	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
	const FVector DebugEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
	AActor* SelectedTarget = IsTargetableActor(HitResult.GetActor()) ? HitResult.GetActor() : nullptr;
	DrawDebugLine(GetWorld(), ViewLocation, DebugEnd, SelectedTarget ? FColor::Green : (bHit ? FColor::Yellow : FColor::Red), 
		false, 1.5f, 0, 1.5f);

	if (SelectedTarget == nullptr)
	{
		SelectedTarget = FindBestAimAssistTarget(ViewLocation, ViewDirection);
	}
	if (SelectedTarget == nullptr)
	{
		ClearTarget();
		return;
	}
	if (CurrentTarget.Get() == SelectedTarget)
	{
		ClearTarget();
		return;
	}

	SetTarget(SelectedTarget);
	const FVector LockOnLocation = ITargetableInterface::Execute_GetLockOnLocation(SelectedTarget);
	DrawDebugLine(GetWorld(), ViewLocation, LockOnLocation, FColor::Cyan, false, 1.5f, 0, 1.5f);
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

bool UTargetingComponent::IsTargetableActor(AActor* TargetActor) const
{
	return IsValid(TargetActor) && TargetActor != OwnerCharacter
		&& TargetActor->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass())
		&& ITargetableInterface::Execute_CanBeTargeted(TargetActor);
}

bool UTargetingComponent::HasClearLineOfSight(AActor* TargetActor, const FVector& ViewLocation) const
{
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnLineOfSight), false, OwnerCharacter);
	QueryParams.AddIgnoredActor(TargetActor);
	const FVector TargetLocation = ITargetableInterface::Execute_GetLockOnLocation(TargetActor);
	return !GetWorld()->LineTraceTestByChannel(ViewLocation, TargetLocation, ECC_Visibility, QueryParams);
}

AActor* UTargetingComponent::FindBestAimAssistTarget(const FVector& ViewLocation, const FVector& ViewDirection) const
{
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnAimAssist), false, OwnerCharacter);
	GetWorld()->OverlapMultiByObjectType(OverlapResults, OwnerCharacter->GetActorLocation(), FQuat::Identity,
		ObjectQueryParams, FCollisionShape::MakeSphere(MaxTargetingDistance), QueryParams);

	const float MinimumAimDot = FMath::Cos(FMath::DegreesToRadians(AimAssistHalfAngle));
	float BestAimDot = MinimumAimDot;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	AActor* BestTarget = nullptr;
	TSet<AActor*> CheckedActors;

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* Candidate = OverlapResult.GetActor();
		if (CheckedActors.Contains(Candidate))
		{
			continue;
		}
		CheckedActors.Add(Candidate);

		if (!IsTargetableActor(Candidate))
		{
			continue;
		}

		const FVector TargetLocation = ITargetableInterface::Execute_GetLockOnLocation(Candidate);
		const FVector ToTarget = TargetLocation - ViewLocation;
		const float DistanceSquared = ToTarget.SizeSquared();
		if (DistanceSquared > FMath::Square(MaxTargetingDistance))
		{
			continue;
		}

		const float AimDot = FVector::DotProduct(ViewDirection, ToTarget.GetSafeNormal());
		const bool bCloserToCrosshair = AimDot > BestAimDot;
		const bool bSameAim = FMath::IsNearlyEqual(AimDot, BestAimDot) && DistanceSquared < BestDistanceSquared;
		if (AimDot < MinimumAimDot || (!bCloserToCrosshair && !bSameAim) || !HasClearLineOfSight(Candidate, ViewLocation))
		{
			continue;
		}

		BestTarget = Candidate;
		BestAimDot = AimDot;
		BestDistanceSquared = DistanceSquared;
	}

	return BestTarget;
}
