#pragma once

#include "NativeGameplayTags.h"

namespace TinoGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action_Attacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action_Dodging);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Action_HitReacting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StatUpgrade_MaxHealth);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StatUpgread_MaxStamina);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StatUpgrade_AttackPower);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_StatUpgrade_Defense);
}