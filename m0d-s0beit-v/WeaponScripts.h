#pragma once

#include "stdafx.h"

__forceinline void BruteForceWeaponAddons(Ped ped, Hash weaponHash, bool bSilencer)
{
	//Since only I get these anyway, might as well craft it for me.
	static Hash weaponAddons[] = { COMPONENT_AT_SCOPE_MACRO, COMPONENT_AT_SCOPE_MACRO_02, COMPONENT_AT_SCOPE_SMALL, COMPONENT_AT_SCOPE_SMALL_02, COMPONENT_AT_SCOPE_MEDIUM, COMPONENT_AT_SCOPE_LARGE, COMPONENT_AT_SCOPE_MAX, COMPONENT_AT_RAILCOVER_01, COMPONENT_AT_AR_AFGRIP, COMPONENT_AT_PI_FLSH, COMPONENT_AT_AR_FLSH, COMPONENT_PISTOL_CLIP_02, COMPONENT_COMBATPISTOL_CLIP_02, COMPONENT_APPISTOL_CLIP_02, COMPONENT_MICROSMG_CLIP_02, COMPONENT_SMG_CLIP_02, COMPONENT_ASSAULTRIFLE_CLIP_02, COMPONENT_CARBINERIFLE_CLIP_02, COMPONENT_ADVANCEDRIFLE_CLIP_02, COMPONENT_MG_CLIP_02, COMPONENT_COMBATMG_CLIP_02, COMPONENT_ASSAULTSHOTGUN_CLIP_02, COMPONENT_PISTOL50_CLIP_02, COMPONENT_ASSAULTSMG_CLIP_02, COMPONENT_SNSPISTOL_CLIP_02, COMPONENT_COMBATPDW_CLIP_02, COMPONENT_HEAVYPISTOL_CLIP_02, COMPONENT_SPECIALCARBINE_CLIP_02, COMPONENT_BULLPUPRIFLE_CLIP_02, COMPONENT_VINTAGEPISTOL_CLIP_02, COMPONENT_MARKSMANRIFLE_CLIP_02, COMPONENT_HEAVYSHOTGUN_CLIP_02, COMPONENT_GUSENBERG_CLIP_02 };
	for each (Hash addonHash in weaponAddons)
	{
		if (WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(weaponHash, addonHash))
			WEAPON::GIVE_WEAPON_COMPONENT_TO_PED(ped, weaponHash, addonHash);
	}
	if (bSilencer)
	{
		//static Hash silencers[] = { COMPONENT_AT_PI_SUPP, COMPONENT_AT_PI_SUPP_02, COMPONENT_AT_AR_SUPP, COMPONENT_AT_SR_SUPP, COMPONENT_AT_AR_SUPP_02 };
		static Hash silencers[] = { COMPONENT_AT_PI_SUPP_02, COMPONENT_AT_AR_SUPP };
		for each (Hash silencerHash in silencers)
		{
			if (WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(weaponHash, silencerHash)) {
				if (weaponHash != WEAPON_ADVANCEDRIFLE && WEAPON::GET_WEAPONTYPE_GROUP(weaponHash) != WEAPON_TYPE_GROUP_SHOTGUN)
					WEAPON::GIVE_WEAPON_COMPONENT_TO_PED(ped, weaponHash, silencerHash);
			}
		}
		if (weaponHash == WEAPON_SNIPERRIFLE || weaponHash == WEAPON_MICROSMG)
			WEAPON::GIVE_WEAPON_COMPONENT_TO_PED(ped, weaponHash, COMPONENT_AT_AR_SUPP_02);
	}
}

__forceinline void GiveAllWeaponsToPed(Ped ped, WeaponTints weaponTint, bool removeWeaponsFirst)
{
	if (removeWeaponsFirst)
		WEAPON::REMOVE_ALL_PED_WEAPONS(ped, TRUE);
	static Hash weaponList[] = { WEAPON_ADVANCEDRIFLE, WEAPON_APPISTOL, WEAPON_ASSAULTRIFLE, WEAPON_ASSAULTSHOTGUN, WEAPON_ASSAULTSMG, WEAPON_BALL, WEAPON_BAT, WEAPON_BOTTLE, WEAPON_BULLPUPSHOTGUN, WEAPON_CARBINERIFLE, WEAPON_COMBATMG, WEAPON_COMBATPDW, WEAPON_COMBATPISTOL, WEAPON_CROWBAR, WEAPON_DAGGER, WEAPON_FIREEXTINGUISHER, WEAPON_FIREWORK, WEAPON_FLAREGUN, WEAPON_GOLFCLUB, WEAPON_GRENADE, WEAPON_GRENADELAUNCHER, WEAPON_GUSENBERG, WEAPON_HAMMER, WEAPON_HEAVYPISTOL, WEAPON_HEAVYSHOTGUN, WEAPON_HEAVYSNIPER, WEAPON_HOMINGLAUNCHER, WEAPON_KNIFE, WEAPON_KNUCKLE, WEAPON_MARKSMANPISTOL, WEAPON_MARKSMANRIFLE, WEAPON_MG, WEAPON_MICROSMG, WEAPON_MOLOTOV, WEAPON_MUSKET, WEAPON_NIGHTSTICK, WEAPON_PETROLCAN, WEAPON_PISTOL, WEAPON_PISTOL50, WEAPON_PROXMINE, WEAPON_PUMPSHOTGUN, WEAPON_RPG, WEAPON_SAWNOFFSHOTGUN, WEAPON_SMG, WEAPON_SMOKEGRENADE, WEAPON_SNIPERRIFLE, WEAPON_SNOWBALL, WEAPON_SNSPISTOL, WEAPON_SPECIALCARBINE, WEAPON_STICKYBOMB, WEAPON_STUNGUN, WEAPON_VINTAGEPISTOL, WEAPON_MINIGUN };
	for each (Hash weapon in weaponList)
	{
		int maxAmmo;
		if (WEAPON::HAS_PED_GOT_WEAPON(ped, weapon, FALSE) == FALSE)
		{
			WEAPON::GIVE_WEAPON_TO_PED(ped, weapon, (WEAPON::GET_MAX_AMMO(ped, weapon, &maxAmmo) == TRUE) ? maxAmmo : 9999, FALSE, TRUE);
			BruteForceWeaponAddons(ped, weapon, true); //This doesn't work for people who are not the player running the commands. You can take their weapons, but if you try to add attachments? FUCK YOU! I AIIIIIIIINNN'T HAVIN' THAT SHIT!
			WEAPON::SET_PED_WEAPON_TINT_INDEX(ped, weapon, ((weapon == WEAPON_MINIGUN) || (weapon == WEAPON_SPECIALCARBINE)) ? WEAPONTINT_PLATINUM : WEAPONTINT_LSPD);
		}
		if (WEAPON::GET_WEAPONTYPE_GROUP(weapon) == WEAPON_TYPE_GROUP_THROWABLE)
		{
			WEAPON::REMOVE_WEAPON_FROM_PED(ped, weapon);
			WEAPON::GIVE_WEAPON_TO_PED(ped, weapon, (WEAPON::GET_MAX_AMMO(ped, weapon, &maxAmmo) == TRUE) ? maxAmmo : 9999, FALSE, TRUE);
		}
	}
}

__forceinline void CheckAndSelectWeapon(Ped ped, Hash weaponHash)
{
	if (WEAPON::HAS_PED_GOT_WEAPON(ped, weaponHash, 0) == FALSE) //If some fucking cunt took our weapons, give them back.
	{
		int maxAmmo;
		WEAPON::GIVE_WEAPON_TO_PED(ped, weaponHash, (WEAPON::GET_MAX_AMMO(ped, weaponHash, &maxAmmo) == TRUE) ? maxAmmo : 9999, FALSE, TRUE);
		WEAPON::SET_PED_WEAPON_TINT_INDEX(ped, weaponHash, (weaponHash == WEAPON_MINIGUN) ? WEAPONTINT_PLATINUM : WEAPONTINT_LSPD);
		BruteForceWeaponAddons(ped, weaponHash, true);
	}
	WEAPON::SET_CURRENT_PED_WEAPON(ped, weaponHash, TRUE);
}