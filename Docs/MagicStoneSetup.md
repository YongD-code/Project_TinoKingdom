# 마력석 파괴 및 퀘스트 설정

## 동작

- 왕이 살아 있으면 플레이어와 몬스터의 피해를 모두 무시합니다.
- 연결된 전투용 왕의 StatComponent.OnDead를 받으면 잠금이 해제됩니다.
- 이후 플레이어의 유효한 공격 판정 3회로 파괴됩니다. 공격력과 방어력은 횟수에 영향을 주지 않습니다.
- Hits To Break를 2로 설정하면 2회로 바뀝니다.
- 몬스터의 공격은 해제 후에도 횟수에 포함하지 않습니다.
- 기존 공격 코드의 Hit Window당 중복 방지를 사용합니다. 여러 Hit Window를 가진 다단히트는 여러 타격으로 셉니다.
- 원본 메시의 월드 위치, 회전, 크기로 Geometry Collection을 생성하고 원본 NPC를 숨깁니다.
- 파편은 지면과 충돌하고 Pawn을 무시하며 기본 8초 후 제거됩니다.
- NPC의 기존 피격 몽타주, 사망 몽타주, 자동 부활은 마력석에 적용하지 않습니다.
- 왕 사망과 파괴 여부는 현재 플레이 세션 상태입니다. 저장 파일에 유지하는 기능은 포함하지 않습니다.

## 에디터 확인

1. CrystalNPC 블루프린트를 엽니다.
2. Components에서 MagicStoneDestruction 컴포넌트를 선택합니다.
3. Hits To Break가 3인지 확인합니다. 원하는 경우 2로 변경합니다.
4. Fracture Meshes에는 magic_crystal/GC_magic_crystal과 magic_crystal1/GC_magic_crystal1 대응이 들어 있습니다.
5. 새 Geometry Collection 컴포넌트를 직접 추가할 필요는 없습니다. 마지막 타격에서 C++이 생성합니다.
6. 레벨의 마력석 인스턴스를 선택하고 파괴 컴포넌트의 King Enemy를 지정할 수 있습니다.
7. King Enemy에는 대화용 NPC가 아닌 실제 전투용 EnemyCharacter를 지정합니다.
8. King Enemy가 비어 있으면 로드된 BossIntroCinematicActor들의 Boss Enemy를 확인합니다. 서로 다른 전투용 보스가 정확히 하나일 때만 자동 연결합니다.
9. 연결할 왕이 없거나 여럿이면 무적 상태를 유지하며 Output Log에 LogMagicStone 오류를 출력합니다. 이 경우 King Enemy를 직접 지정합니다.

자동 연결은 BeginPlay 다음 틱에 한 번 수행합니다. 나중에 스트리밍되거나 생성되는 왕은 별도 연결 처리가 필요합니다.
파괴 위치가 어긋나면 GC 에셋 자체의 원점이 원본 Static Mesh와 일치하는지 확인합니다.
GC에 조각이 실제로 분할되어 있어야 합니다. 생성만 하고 Fracture를 하지 않은 에셋은 조각으로 흩어지지 않습니다.
묶음이 풀리지 않으면 GC의 Damage Threshold와 컴포넌트의 Break Strain을 비교합니다.

## 대화로 퀘스트 받기: 직접 설정할 부분

퀘스트 데이터와 대사 데이터는 이번 작업에서 수정하지 않았습니다.

1. 콘텐츠 브라우저에서 우클릭 → Miscellaneous → Data Asset을 선택합니다.
2. 클래스 선택 창에서 QuestData를 선택하고 DA_Quest_MagicStone을 만듭니다.
3. Quest Id에 중복되지 않는 이름을 입력합니다. 예: MagicStoneQuest.
4. Title, Description에 표시할 제목과 설명을 입력합니다.
5. Objective Type을 Item 또는 Cooking으로 선택하고 해당 목표와 Required Count를 지정합니다.
6. Reward Type과 보상 내용을 지정합니다.
7. CrystalNPC → Class Defaults → Quest → Quest To Grant에 DA_Quest_MagicStone을 지정합니다.
8. Dialogue → Dialogue Data에 퀘스트 제안 대사를 지정합니다. 기존 DA_Crystal을 사용할 경우 실제 대사 내용을 확인합니다.
9. In Progress Dialogue Data에는 진행 중 대사, Ready To Complete Dialogue Data에는 보고 대사, Completed Dialogue Data에는 완료 후 대사를 지정합니다.
10. Compile 후 플레이에서 상호작용키로 대화를 끝까지 진행합니다.

현재 DialogueComponent는 대화 완료 시 GetActiveQuest로 퀘스트를 찾고, NotStarted이면 AcceptQuest를 호출합니다.
별도 Event Graph에서 Accept Quest를 중복 연결할 필요가 없습니다. 대화를 시작하는 즉시가 아니라 끝낸 시점에 받습니다.
파괴 때문에 강제로 종료된 대화는 퀘스트 수락/완료로 처리하지 않습니다.

현재 QuestData의 Objective Type은 Item과 Cooking만 지원합니다.
"왕 처치"나 "마력석 파괴"를 직접 목표로 추적하려면 퀘스트 목표 타입과 진행 이벤트 연결을 별도로 확장해야 합니다.
이번 파괴 컴포넌트의 On Stone Broken 이벤트는 그 연결 지점으로 사용할 수 있지만 퀘스트 완료 처리는 아직 연결하지 않았습니다.
마력석을 파괴한 후 그 마력석에 다시 보고할 수 없으므로, 파괴 퀘스트라면 자동 완료 또는 다른 NPC에게 보고하는 규칙도 필요합니다.

## 플레이 검증 순서

1. 왕 생존 중 5회 공격: Received Hits가 0이고 파괴되지 않아야 합니다.
2. 실제 왕을 처치: LogMagicStone에 King defeated; stone unlocked가 출력되어야 합니다.
3. 플레이어 1~2회 공격: 원본 메시가 유지되어야 합니다.
4. 세 번째 공격: NPC가 숨겨지고 파편이 생성되어야 합니다.
5. 추가 공격: 파편이 중복 생성되지 않아야 합니다.
6. 약 8초 후 파편이 제거되어야 합니다.
7. Hits To Break를 2로 바꾸고 다시 시작해 두 번째 타격에 파괴되는지 확인합니다.

## 파일

- Component/MagicStoneDestructionComponent.h/.cpp: 왕 연결, 타격 횟수, GC 생성과 파괴.
- Character/TinoNPCCharacter.cpp: 컴포넌트가 있는 NPC의 피해/사망 판정을 위임.
- Component/DialogueComponent.h/.cpp: 파괴된 NPC와의 대화만 취소.
- World/BossIntroCinematicActor.h: 기존 전투용 보스 참조 접근자.
- Scripts/setup_magic_stone.py: 기존 CrystalNPC에 파괴 컴포넌트를 한 번만 추가하는 에디터 스크립트.
- Saved/MagicStoneBackup/CrystalNPC.uasset: 스크립트가 만든 수정 전 블루프린트 백업.
