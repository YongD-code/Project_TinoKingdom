#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "MassEntityConfigAsset.h"
#include "Project_TinoKingdom/Character/EnemyCharacter.h"
#include "Project_TinoKingdom/World/TinoEndingCrowdSpawner.h"
#include "Project_TinoKingdom/World/TinoEndingCrowdSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTinoEndingCrowdStreamingRegistrationTest,
	"TinoKingdom.EndingCrowd.StreamingRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTinoEndingCrowdStreamingRegistrationTest::RunTest(const FString& Parameters)
{
	// 렌더링이나 Mass 생성은 실행하지 않고 엔딩 상태와 로드 등록의 수명만 검사합니다.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("테스트 월드 생성"), World))
	{
		return false;
	}
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	UTinoEndingCrowdSubsystem* Session = World->GetSubsystem<UTinoEndingCrowdSubsystem>();
	if (!TestNotNull(TEXT("월드 관리자 생성"), Session))
	{
		return false;
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATinoEndingCrowdSpawner* Settings = World->SpawnActor<ATinoEndingCrowdSpawner>();
	AEnemyCharacter* NearEnemy = World->SpawnActor<AEnemyCharacter>(AEnemyCharacter::StaticClass(),
		FTransform(FVector::ZeroVector), SpawnParameters);
	AEnemyCharacter* FarEnemy = World->SpawnActor<AEnemyCharacter>(AEnemyCharacter::StaticClass(),
		FTransform(FVector(600000.0, 0.0, 0.0)), SpawnParameters);
	AEnemyCharacter* UntaggedEnemy = World->SpawnActor<AEnemyCharacter>(AEnemyCharacter::StaticClass(),
		FTransform(FVector(300000.0, 0.0, 0.0)), SpawnParameters);
	if (!TestNotNull(TEXT("설정 스포너"), Settings) || !TestNotNull(TEXT("가까운 몬스터"), NearEnemy)
		|| !TestNotNull(TEXT("먼 몬스터"), FarEnemy) || !TestNotNull(TEXT("태그 없는 몬스터"), UntaggedEnemy))
	{
		return false;
	}
	const FName Tag(TEXT("EndingCrowdTarget"));
	NearEnemy->Tags.Add(Tag);
	FarEnemy->Tags.Add(Tag);
	const bool bOriginalDamage = NearEnemy->CanBeDamaged();
	const bool bOriginalCollision = NearEnemy->GetActorEnableCollision();
	Session->RegisterEnemy(*NearEnemy);
	TestEqual(TEXT("엔딩 전 등록은 대체 군중을 만들지 않음"), Session->GetPendingCount(), 0);
	TestFalse(TEXT("엔딩 전에는 몬스터를 숨기지 않음"), NearEnemy->IsHidden());

	FString Error;
	TestFalse(TEXT("잘못된 설정으로 엔딩이 활성화되지 않음"),
		Session->ActivateEnding(*Settings, Tag, FVector(50.0), 60.0f, Error));
	TestFalse(TEXT("설정 실패 후 엔딩 상태 유지 안 함"), Session->IsEndingActive());
	UMassEntityConfigAsset* Config = NewObject<UMassEntityConfigAsset>(World);
	Settings->SetEndingEntityConfig(TSoftObjectPtr<UMassEntityConfigAsset>(Config));
	if (!TestTrue(TEXT("엔딩 활성화"), Session->ActivateEnding(*Settings, Tag, FVector(50.0), 60.0f, Error)))
	{
		return false;
	}
	TestTrue(TEXT("중복 활성화는 같은 상태 재사용"),
		Session->ActivateEnding(*Settings, Tag, FVector(50.0), 60.0f, Error));
	TestFalse(TEXT("활성화 후 다른 태그로 상태를 덮어쓰지 않음"),
		Session->ActivateEnding(*Settings, TEXT("DifferentTag"), FVector(50.0), 60.0f, Error));

	// 실제 스트리밍 콜백과 동일한 등록 함수를 호출하며 위치에 따른 필터가 없는지 확인합니다.
	Session->RegisterEnemy(*NearEnemy);
	Session->RegisterEnemy(*FarEnemy);
	Session->RegisterEnemy(*UntaggedEnemy);
	TestEqual(TEXT("거리와 무관하게 태그 대상 두 마리만 등록"), Session->GetPendingCount(), 2);
	TestTrue(TEXT("늦게 로드된 몬스터를 즉시 숨김"), FarEnemy->IsHidden());
	TestTrue(TEXT("늦게 로드된 몬스터의 AI 차단"), FarEnemy->IsCinematicAIBlocked());
	TestFalse(TEXT("늦게 로드된 몬스터의 충돌 차단"), FarEnemy->GetActorEnableCollision());
	TestFalse(TEXT("늦게 로드된 몬스터의 잠금 대상 선택 차단"), FarEnemy->CanBeTargeted_Implementation());
	FarEnemy->SetCinematicAIBlocked(false);
	TestTrue(TEXT("다른 시네마틱의 해제 요청으로 엔딩 AI 차단을 풀지 않음"), FarEnemy->IsCinematicAIBlocked());
	TestFalse(TEXT("태그 없는 몬스터는 숨기지 않음"), UntaggedEnemy->IsHidden());
	Session->RegisterEnemy(*NearEnemy);
	TestEqual(TEXT("같은 액터를 다시 등록해도 중복되지 않음"), Session->GetPendingCount(), 2);

	Session->UnregisterEnemy(*NearEnemy);
	TestEqual(TEXT("언로드한 몬스터의 등록 정리"), Session->GetPendingCount(), 1);
	TestTrue(TEXT("언로드 이후에도 월드 엔딩 상태 유지"), Session->IsEndingActive());
	TestFalse(TEXT("재사용할 액터의 표시 상태 복원"), NearEnemy->IsHidden());
	TestEqual(TEXT("재사용할 액터의 피해 설정 복원"), NearEnemy->CanBeDamaged(), bOriginalDamage);
	TestEqual(TEXT("재사용할 액터의 충돌 설정 복원"), NearEnemy->GetActorEnableCollision(), bOriginalCollision);
	Session->UnregisterEnemy(*NearEnemy);
	TestEqual(TEXT("등록 해제 중복 호출 안전"), Session->GetPendingCount(), 1);
	Session->RegisterEnemy(*NearEnemy);
	TestTrue(TEXT("같은 액터가 재로드되어도 사람 대기 상태 적용"), NearEnemy->IsHidden());
	TestEqual(TEXT("재로드 후에도 대상별 등록 한 개 유지"), Session->GetPendingCount(), 2);
	Settings->Destroy();
	TestTrue(TEXT("배치된 설정 스포너가 제거되어도 월드 상태 유지"), Session->IsEndingActive());

	Session->OnWorldEndPlay(*World);
	TestFalse(TEXT("월드 종료 시 엔딩 상태 정리"), Session->IsEndingActive());
	TestEqual(TEXT("월드 종료 시 모든 등록 정리"), Session->GetPendingCount(), 0);
	return true;
}

#endif
