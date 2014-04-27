///////////// Copyright © 2008, Killer Monkey. All rights reserved. /////////////
// 
// File: weapon_sniperrifle.cpp
// Description:
//      Implimentation of the Sniper Rifle weapon.
//
// Created On: 11/1/2008
// Created By: Killer monkey
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
#define CWeaponSniper C_WeaponSniper
#endif

//-----------------------------------------------------------------------------
// CWeaponSniper
//-----------------------------------------------------------------------------

class CWeaponSniper : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponSniper, CGEWeaponPistol );
	
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponSniper(void)
	{
		SetAlwaysSilenced(true);
		// NPC Ranging
		m_fMinRange1 = 100;
		m_fMaxRange1 = 8000;

#ifdef CLIENT_DLL
		// Hard coded since Valve sucks
		m_iLastZoomOffset = -80;
#endif
	}

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_SNIPER_RIFLE; }
	
#ifdef CLIENT_DLL
	virtual int KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
	{
		C_GEPlayer *pPlayer = (C_GEPlayer*) C_GEPlayer::GetLocalPlayer();

		if ( pPlayer && (keynum == MOUSE_WHEEL_UP || keynum == MOUSE_WHEEL_DOWN) )
		{
			// Check if we are allowed to zoom in/out
			if ( pPlayer->IsInAimMode() || pPlayer->GetAimModeState() != AIM_NONE )
			{
				// Grab our current desired zoom
				int cur_zoom = pPlayer->GetZoomEnd();

				// Modify the zoom based on our input
				if ( keynum == MOUSE_WHEEL_UP )
					cur_zoom -= 5;
				else
					cur_zoom += 5;
			
				// Clamp zoom to our default fov and the max sniper rifle zoom
				// NOTE: We use direct access to GEWpnData since we modify the accessor function below
				cur_zoom = clamp( cur_zoom, (90 + GetGEWpnData().m_iZoomOffset) - pPlayer->GetDefaultFOV(), 0 );
			
				// Apply the new zoom to the player
				pPlayer->SetZoom( cur_zoom );

				// Save it away
				m_iLastZoomOffset = cur_zoom;

				// Gobble up the key press
				return 0;
			}
		}
		
		return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
	}

	virtual float GetZoomOffset()
	{
		return m_iLastZoomOffset;
	}

	virtual bool Holster( C_BaseCombatWeapon *pSwitchingTo )
	{
		// Reset the last zoom change
		if ( BaseClass::Holster( pSwitchingTo ) )
		{
			m_iLastZoomOffset = GetGEWpnData().m_iZoomOffset;
			return true;
		}

		return false;
	}

private:
	int m_iLastZoomOffset;
#endif

private:
	CWeaponSniper( const CWeaponSniper & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSniper, DT_WeaponSniper )

BEGIN_NETWORK_TABLE( CWeaponSniper, DT_WeaponSniper )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponSniper )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_sniper_rifle, CWeaponSniper );
PRECACHE_WEAPON_REGISTER( weapon_sniper_rifle );

acttable_t CWeaponSniper::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_SNIPERRIFLE,				false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_AR2,				false },

	{ ACT_MP_RUN,						ACT_GES_RUN_SNIPERRIFLE,				false },
	{ ACT_MP_WALK,						ACT_GES_WALK_SNIPERRIFLE,				false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_AR2,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PHANTOM,	false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PHANTOM,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PHANTOM,	false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_RIFLE,				false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_AR33,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_AR33,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_SNIPERRIFLE,				false },
};
IMPLEMENT_ACTTABLE( CWeaponSniper );
