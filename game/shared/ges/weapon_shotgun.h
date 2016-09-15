///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_shotgun.cpp
// Description:
//       Header file for shotguns.  Autoshotgun inherits from shotgun so just make changes there.
//
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_ge_player.h"
#else
#include "ge_player.h"
#endif

#include "ge_weaponpistol.h"

#ifdef CLIENT_DLL
#define CWeaponShotgun C_WeaponShotgun
#endif


class CWeaponShotgun : public CGEWeaponPistol
{
public:
	DECLARE_CLASS(CWeaponShotgun, CGEWeaponPistol);

	CWeaponShotgun(void);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual void PrimaryAttack(void);
	virtual void AddViewKick(void);

	// Functions to add ammo count dependant shells
	virtual void OnReloadOffscreen(void);
	virtual bool Deploy(void);
	virtual void Precache(void);

	virtual void CalculateShellVis(bool fillclip);

	// Override pistol behavior of recoil fire animation
	virtual Activity GetPrimaryAttackActivity(void) { return ACT_VM_PRIMARYATTACK; };

	virtual GEWeaponID GetWeaponID(void) const { return WEAPON_SHOTGUN; }
	virtual bool	IsShotgun() { return true; };

#ifdef GAME_DLL
	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
#endif

	DECLARE_ACTTABLE();

private:
	CWeaponShotgun(const CWeaponShotgun &);
	int	m_iShellBodyGroup[5];
};