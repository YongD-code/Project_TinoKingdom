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
}