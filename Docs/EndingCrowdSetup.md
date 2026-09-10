# 엔딩 군중 전환 테스트

현재 구현 범위는 `EndingCrowdTarget` 액터 태그가 붙은 살아 있는 `EnemyCharacter`를 찾아 발밑 NavMesh 위치에 사람을 한 명씩 생성하는 것까지입니다. 생성 수를 확인한 뒤 원본 몬스터의 AI, 피해, 잠금 대상 선택, 표시, 충돌, 액터 틱을 끕니다. 몬스터를 사망 처리하거나 파괴하지 않습니다.

이펙트, 엔딩 시퀀스 연결, 재접속 후 상태 유지, 멀티플레이 동기화는 이번 범위에 포함하지 않습니다. 빌드와 실행은 사용자가 진행합니다.

## 에디터 설정

1. 에디터를 닫고 프로젝트의 Editor 타깃을 빌드한 다음 에디터에서 `TinoKingdom_ByChanWoong` 맵을 엽니다. 새 클래스와 프로퍼티가 추가되어 있으므로 기존 에디터 세션의 Live Coding만으로 확인하지 않습니다.
2. 새 **TinoEndingCrowdSpawner** 액터를 하나 배치합니다. 이전 테스트의 **MetaHuman Mass Spawner**와 다른 클래스입니다.
3. 새 스포너의 `Entity Types`를 하나만 추가하고 `Entity Config`에 `DA_TinoCrowdEnding`, `Proportion`에 `1.0`을 지정합니다. `Auto Spawn on Begin Play`는 끕니다. `Spawn Data Generators`는 비워둡니다. 코드가 요청 시 전용 생성기를 넣으며 Count는 대상 몬스터 수로 정합니다. 이번 전환에는 EQS를 사용하지 않습니다.
4. 기존 `TinoEndingCrowdController` 액터의 `Crowd Spawner`를 이 새 스포너로 변경합니다. 이전 테스트 스포너는 삭제할 필요 없이 자동 생성을 끈 상태로 두면 됩니다.
5. 제어 액터에서 `Transform Tagged Monsters`를 켜고 `Target Monster Tag`는 `EndingCrowdTarget`으로 둡니다. 테스트할 몬스터 한 마리의 **Actor > Tags**에도 같은 태그가 있어야 합니다. Gameplay Tags나 Component Tags가 아닙니다.
6. 제어 액터에서 `Test Spawn on Begin Play`를 켜고 `Test Spawn Delay`는 `5.0`으로 둡니다. 몬스터와 제어 액터, 스포너가 플레이 중 로드되는 영역에 있어야 하며 몬스터 발밑에 NavMesh가 있어야 합니다.
7. 맵을 저장하고 직접 플레이합니다. 테스트가 끝나면 `Test Spawn on Begin Play`를 다시 끕니다. 이후 엔딩 코드에서 `SpawnEndingCrowd()`를 호출하도록 연결할 예정입니다.

## 직접 확인할 항목

- 정상: 요청 시 대상 몬스터가 AI 정지 상태로 기다린 뒤 사람이 한 명 생성되고 몬스터가 숨겨집니다. 제어 액터의 런타임 `Monsters Transformed`와 `Spawning Finished`가 참이어야 합니다. 사람의 배회 동작과 외형은 연결된 Entity Config와 StateTree를 따릅니다.
- 중복 방지: 같은 플레이에서 다시 요청해도 사람이 추가 생성되지 않아야 합니다.
- 태그 없음 또는 NavMesh 밖: `Last Spawn Error`에 이유가 표시되고 몬스터가 숨겨지지 않아야 합니다. 위치 탐색 범위 기본값은 X/Y 50cm, Z 200cm입니다.
- 생성 실패 또는 시간 초과: 생성 중인 군중을 취소하고 몬스터의 이전 AI 차단 상태와 피해 허용 상태를 복원합니다. 이전 전투 대상이나 진행 중이던 공격까지 복원하는 기능은 아닙니다. 기본 제한 시간은 게임 시간 60초이며, 재시도는 플레이를 종료하고 다시 시작합니다.
- 로그는 Output Log에서 `LogTinoEndingCrowd`로 찾을 수 있습니다. 처음에는 태그 대상 한 마리로 확인하고 그다음 수를 늘립니다.

## 주의 사항

- Mass 엔티티 생성 완료는 화면 표시나 애니메이션 준비 완료를 보장하지 않습니다. 첫 실행의 로딩 지연이나 전환 순간의 겹침을 숨기는 이펙트 및 사전 준비는 다음 단계에서 다룹니다.
- 위치는 생성 요청 시점의 발밑을 NavMesh에 투영한 값입니다. 사람은 생성 직후 StateTree에 따라 움직일 수 있습니다. 몬스터의 캡슐 중심 높이나 크기는 복사하지 않습니다.
- 현재 월드에 로드된 살아 있고 숨겨지지 않은 `EnemyCharacter`만 대상입니다. 죽은 몬스터, 미로드 영역, 나중에 생성되는 몬스터는 이번 요청에 포함되지 않습니다.
- 전환 후에도 원본 몬스터 액터는 숨겨진 채 남습니다. 사람들은 스포너가 관리합니다. 테스트 중 스포너를 직접 DEBUG 생성/제거하거나 다른 제어 액터와 공유하지 않습니다.
- 이번 변경은 정적 검토만 진행했습니다. 컴파일, PIE 및 실제 군중 표시 검증은 아직 수행하지 않았습니다.
