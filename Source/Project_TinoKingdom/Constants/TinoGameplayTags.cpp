#include "TinoGameplayTags.h"

namespace TinoGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Action_Attacking,
		"State.Action.Attacking",
		"캐릭터가 공격 몽타주를 실행 중"
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Action_Dodging,
		"State.Action.Dodging",
		"캐릭터가 구르기 동작을 실행 중"
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Action_HitReacting,
		"State.Action.HitReacting",
		"캐릭터가 피격 반응 중"
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Dead,
		"State.Dead",
		"캐릭터가 사망한 상태"
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Invincible,
		"State.Invincible",
		"캐릭터가 피해를 받지 않는 상태"
	);
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Data_Damage,
		"Data.Damage",
		"SetByCaller를 통해 전달되는 피해량"
	);
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
	Data_StatUpgrade_MaxHealth,
	"Data.StatUpgrade.MaxHealth",
	"SetByCaller를 통해 전달되는 최대 체력 증가량"
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Data_StatUpgrade_MaxStamina,
		"Data.StatUpgrade.MaxStamina",
		"SetByCaller를 통해 전달되는 최대 기력 증가량"
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Data_StatUpgrade_AttackPower,
		"Data.StatUpgrade.AttackPower",
		"SetByCaller를 통해 전달되는 공격력 증가량"
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Data_StatUpgrade_Defense,
		"Data.StatUpgrade.Defense",
		"SetByCaller를 통해 전달되는 방어력 증가량"
	);
}