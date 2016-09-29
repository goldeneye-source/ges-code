///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_player.cpp
// Description:
//      GoldenEye Client Player Class.
//
// Created On: 23 Feb 08, 17:06
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "vcollide_parse.h"
#include "c_ge_player.h"
#include "c_gemp_player.h"
#include "ge_utils.h"
#include "gemp_gamerules.h"
#include "view.h"
#include "takedamageinfo.h"
#include "ge_gamerules.h"
#include "in_buttons.h"
#include "iviewrender_beams.h"	// flashlight beam
#include "r_efx.h"
#include "dlight.h"
#include "achievementmgr.h"
#include "ge_vieweffects.h"
#include "voice_status.h"

// For special music setup
#include "ge_musicmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Don't alias here
#if defined( CGEPlayer )
#undef CGEPlayer	
#endif

// We don't like this to "player" since we use a custom mp_player

IMPLEMENT_CLIENTCLASS_DT(C_GEPlayer, DT_GE_Player, CGEPlayer)
	RecvPropEHandle( RECVINFO( m_hActiveLeftWeapon ) ),
	RecvPropInt( RECVINFO( m_ArmorValue ) ),
	RecvPropInt( RECVINFO( m_iMaxArmor ) ),
	RecvPropInt( RECVINFO( m_iMaxHealth ) ),

	RecvPropFloat( RECVINFO( m_flFullZoomTime ) ),

	RecvPropEHandle( RECVINFO( m_hHat ) ),
	RecvPropInt( RECVINFO( m_takedamage ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_GEPlayer )
DEFINE_PRED_FIELD( m_flFullZoomTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

extern ConVar v_viewmodel_fov;

static ConVar ge_fp_ragdoll ( "ge_fp_ragdoll", "1", FCVAR_ARCHIVE, "Allow first person ragdolls" );
static ConVar ge_fp_ragdoll_auto ( "ge_fp_ragdoll_auto", "1", FCVAR_ARCHIVE, "Autoswitch to ragdoll thirdperson-view when necessary" );
ConVar cl_ge_nohitothersound( "cl_ge_nohitsound", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Disable the sound that plays when you damage another player" );
ConVar cl_ge_nokillothersound( "cl_ge_nokillsound", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Disable the sound that plays when you kill another player" );
ConVar cl_ge_nowepskins("cl_ge_nowepskins", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Disables alternative weapon appearances.");
ConVar cl_ge_hidetags("cl_ge_hidetags", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Disables BT/Dev/CC tags.");

C_GEPlayer::C_GEPlayer()
{
	ListenForGameEvent( "specialmusic_setup" );
	ListenForGameEvent( "round_end" );

	m_bInSpecialMusic = false;
	m_flEndSpecialMusic = 0.0f;
	m_iMaxArmor = MAX_ARMOR;
	m_iMaxHealth = MAX_HEALTH;

	m_iNewZoomOffset = 0;
	m_bSentUnlockCode = false;

	// Load our voice overhead icons if not done yet
	if ( m_pDM_VoiceHeadMaterial == NULL )
	{
		m_pDM_VoiceHeadMaterial = materials->FindMaterial( "sprites/hud/ic_voice_dm", TEXTURE_GROUP_VGUI );
		m_pDM_VoiceHeadMaterial->IncrementReferenceCount();

		m_pMI6_VoiceHeadMaterial = materials->FindMaterial( "sprites/hud/ic_voice_mi6", TEXTURE_GROUP_VGUI );
		m_pMI6_VoiceHeadMaterial->IncrementReferenceCount();

		m_pJanus_VoiceHeadMaterial = materials->FindMaterial( "sprites/hud/ic_voice_janus", TEXTURE_GROUP_VGUI );
		m_pJanus_VoiceHeadMaterial->IncrementReferenceCount();
	}
}

C_GEPlayer::~C_GEPlayer( void )
{
	// Make sure we resume our music if we get destroyed right before level load
	GEMusicManager()->ResumeMusic();

	m_pDM_VoiceHeadMaterial->DecrementReferenceCount();
	m_pMI6_VoiceHeadMaterial->DecrementReferenceCount();
	m_pJanus_VoiceHeadMaterial->DecrementReferenceCount();
}

IMaterial *C_GEPlayer::GetHeadLabelMaterial( void )
{
	if ( GetClientVoiceMgr() == NULL )
		return NULL;

	// Return our talk material based on our current team
	if ( GetTeamNumber() == TEAM_MI6 )
		return m_pMI6_VoiceHeadMaterial;
	else if ( GetTeamNumber() == TEAM_JANUS )
		return m_pJanus_VoiceHeadMaterial;
	else
		return m_pDM_VoiceHeadMaterial;
}

float C_GEPlayer::GetHeadOffset( void )
{
	float hat_offset = 0;
	if ( GetHat() != NULL )
		hat_offset = 3.0f;

	if ( GetFlags() & FL_DUCKING )
	{
		// Check for duck jump
		if ( GetFlags() & FL_ONGROUND )
			return 15.0f + hat_offset;
		else
			return 25.0f + hat_offset;	
	}

	return hat_offset;
}

void C_GEPlayer::SetZoom( int zoom, bool forced /*=false*/ )
{
	float zoom_time = forced ? 0 : abs(zoom - GetZoom()) / WEAPON_ZOOM_RATE;
	Zoom( zoom, zoom_time );
}

int C_GEPlayer::GetZoomEnd()
{
	return m_flZoomEnd;
}

int C_GEPlayer::GetAimModeState()
{
	if (m_flFullZoomTime > 0)
	{
		if (m_flFullZoomTime < gpGlobals->curtime)
			return AIM_ZOOMED;
		else
			return AIM_ZOOM_IN;
	}
	return AIM_NONE;
}

void C_GEPlayer::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	// if we're dead, we want to deal with first or third person ragdolls.
	if ( m_lifeState != LIFE_ALIVE && !IsObserver() )
	{
		// First person ragdolls
		if ( ge_fp_ragdoll.GetBool() && GetRagdoll() )
		{
			// pointer to the ragdoll
			C_HL2MPRagdoll *pRagdoll = GetRagdoll();

			// gets its origin and angles
			pRagdoll->GetAttachment( pRagdoll->LookupAttachment( "eyes" ), eyeOrigin, eyeAngles );
			Vector vForward; 
			AngleVectors( eyeAngles, &vForward );

			if ( ge_fp_ragdoll_auto.GetBool() )
			{
				// DM: Don't use first person view when we are very close to something
				trace_t tr;
				UTIL_TraceLine( eyeOrigin, eyeOrigin + ( vForward * 25 ), MASK_ALL, pRagdoll, COLLISION_GROUP_NONE, &tr );

				if ( tr.fraction == 1.0f || tr.startsolid )
					return;
			}
			else
				return;
		}
	}

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov);
}

bool C_GEPlayer::CanSetSoundMixer( void )
{
	if ( m_bInSpecialMusic )
		return false;

	return BaseClass::CanSetSoundMixer();
}

void C_GEPlayer::PreThink(void)
{
	CheckAimMode();

	BaseClass::PreThink();
}

void C_GEPlayer::FireGameEvent( IGameEvent *event )
{
	const char *name = event->GetName();
	if ( Q_strcmp( name, "specialmusic_setup" ) == 0 )
	{
		// See if we should worry about this
		int plID = event->GetInt("playerid");
		int tmID = event->GetInt("teamid");
		
		// An ID passed as -1 means "for all players" but we can also team restrict
		if ( plID == GetUserID() || ( plID == -1 && tmID == GetTeamNumber() ) || ( plID == -1 && tmID == -1 ) )
		{
			float duration = GetSoundDuration( event->GetString("soundfile"), NULL );
			if ( duration < 0.5f )
				return;

			// Remove 0.5 seconds for transition time
			if ( duration > 2.5f )
				duration -= 0.5f;

			// Don't allow more than 20 seconds of special music muting
			duration = min( duration, 20.0f );

			// Kick it off!
			SetupSpecialMusic( duration );
		}
	}
	else if (!Q_stricmp(name, "round_end"))
	{
		m_iNewZoomOffset = 0;  // When the round ends, unzoom.
	}
}

void C_GEPlayer::SetupSpecialMusic( float duration )
{
	m_flEndSpecialMusic = gpGlobals->curtime + duration;
}

void C_GEPlayer::CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );

	GEViewEffects()->ApplyBreath();
}

void C_GEPlayer::AdjustCollisionBounds( bool inflate )
{
	if ( inflate )
	{
		// Modify Collision Bounds for trace hits
		if ( GetFlags() & FL_DUCKING )
			SetCollisionBounds( VEC_DUCK_SHOT_MIN, VEC_DUCK_SHOT_MAX );
		else
			SetCollisionBounds( VEC_SHOT_MIN, VEC_SHOT_MAX );
	}
	else
	{
		// Put the collision bounds back
		if ( GetFlags() & FL_DUCKING )
			SetCollisionBounds( VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
		else
			SetCollisionBounds( VEC_HULL_MIN, VEC_HULL_MAX );
	}
}

bool C_GEPlayer::ShouldDraw( void )
{
	// If we're dead, our ragdoll will be drawn for us instead.
	if ( IsObserver() )
		return false;

	// Draw us if we are a ragdoll
	if( IsLocalPlayer() && IsRagdoll() )
		return true;

	// Don't draw if we are spectating
	if( GetTeamNumber() == TEAM_SPECTATOR || IsRagdoll() )
		return false;

	// Skip over HL2MP's definition
	return C_BasePlayer::ShouldDraw();
}

void C_GEPlayer::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();

	if ( !m_bInSpecialMusic && gpGlobals->curtime < m_flEndSpecialMusic )
	{
		// Force the sound mixer to the Death Music so it can be heard better
		ConVar *pVar = (ConVar *)cvar->FindVar( "snd_soundmixer" );
		pVar->SetValue( "GE_SpecialMusic" );
		m_bInSpecialMusic = true;

		GEMusicManager()->PauseMusic();
	}
	else if ( m_bInSpecialMusic && gpGlobals->curtime >= m_flEndSpecialMusic )
	{
		// Force the sound mixer to the default (in case we don't have soundscapes)
		// if we do have soundscapes this will be overwritten anyway
		ConVar *pVar = (ConVar *)cvar->FindVar( "snd_soundmixer" );
		pVar->SetValue( "Default_Mix" );
		m_bInSpecialMusic = false;

		GEMusicManager()->ResumeMusic();
	}
}

void C_GEPlayer::ClientThink( void )
{
	if ( IsLocalPlayer() )
	{
		// See if we need to zoom in/out based on our aim mode

		// We have to do this here rather than in aimmode land so gpGlobals->curtime reads off the correct value.
		// But we have to check aimmode status in preframe because it needs to be called in the same function as on the server
		// and the server has no ClientThink.

		if (IsObserver())
		{
			C_GEPlayer *pGEObsTarget = ToGEPlayer(GetObserverTarget());

			if ( GetObserverMode() == OBS_MODE_IN_EYE && pGEObsTarget )
			{
				CGEWeapon *pGEWeapon = ToGEWeapon(pGEObsTarget->GetActiveWeapon());

				if ( pGEWeapon && pGEObsTarget->StartedAimMode() )
					m_iNewZoomOffset = pGEWeapon->GetZoomOffset();
				else
					m_iNewZoomOffset = 0;

				if ( m_iNewZoomOffset != GetZoomEnd() )
				{
					SetZoom( m_iNewZoomOffset );
					Warning("Set zoom to %d!\n", m_iNewZoomOffset);
				}
			}
			else
			{
				m_iNewZoomOffset = 0;
				SetZoom(0, true);
			}
		}
		else if (!IsAlive()) //Force reset our zoom if we died right away, otherwise we wrap around to the next frame and can't reset zoom since we're already fully dead.
		{
			m_iNewZoomOffset = 0;
			SetZoom(0, true);
		}
		else if (GetActiveWeapon() && m_iNewZoomOffset != 0 && m_iNewZoomOffset != ToGEWeapon(GetActiveWeapon())->GetZoomOffset()) // Dinky fix because dangit I give up.  Sometimes the reset zoom conditions just flat out fail and the client stays zoomed.
		{
			m_iNewZoomOffset = StartedAimMode() ? ToGEWeapon(GetActiveWeapon())->GetZoomOffset() : 0;
			SetZoom(m_iNewZoomOffset);
		}
		else if (m_iNewZoomOffset != m_flZoomEnd)
			SetZoom(m_iNewZoomOffset);

		// Check if we should draw our hat
		if ( m_hHat.Get() )
		{
			if ( !ShouldDrawLocalPlayer() ) //IsAlive()
				m_hHat.Get()->AddEffects( EF_NODRAW );
			else
				m_hHat.Get()->RemoveEffects( EF_NODRAW );
		}
		
		// Adjust the weapon's FOV based on our own FOV
		// This is basically a dinky fix for weapon FOV calculations sticking the FOV out of bounds and placing the weapon
		// in the upper left corner of the screen.

		C_BaseViewModel *vm = GetViewModel();

		if ( vm )
		{
			if (54.0f + GetZoom() <= 0)
				v_viewmodel_fov.SetValue(GetZoom() * -1 + 5);
			else if (v_viewmodel_fov.GetFloat() != 54.0)
				v_viewmodel_fov.SetValue(54.0f);
		}
	}

	BaseClass::ClientThink();
}

void C_GEPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// Looks like we spawned, cancel any sound modifiers that are active
	if ( m_iSpawnInterpCounter != m_iSpawnInterpCounterCache )
	{
		if ( IsLocalPlayer() )
		{
			if ( m_bInSpecialMusic )
				m_flEndSpecialMusic = gpGlobals->curtime;

			GEViewEffects()->ResetBreath();
			m_iNewZoomOffset = 0;
		}
	}

	BaseClass::PostDataUpdate( updateType );

	// This occurs after the baseclass cause we have to setup the local player first
	if ( updateType == DATA_UPDATE_CREATED && IsLocalPlayer() )
	{
		CAchievementMgr *pAchievementMgr = static_cast<CAchievementMgr*>(engine->GetAchievementMgr());
		if ( !pAchievementMgr )
			return;

		pAchievementMgr->SendAchievementProgress();
	}

	// Check if we changed weapons to reset zoom
	if ( !m_bSentUnlockCode && IsLocalPlayer() && ToGEMPPlayer(this)->GetSteamHash())
	{
		if ( GEMPRules()->GetSpecialEventCode() )
			GEUTIL_WriteUniqueSkinData(GEUTIL_EventCodeToSkin(GEMPRules()->GetSpecialEventCode()), ToGEMPPlayer(this)->GetSteamHash());

		char cmd[128];
		Q_snprintf(cmd, sizeof(cmd), "skin_unlock_code %llu", GEUTIL_GetUniqueSkinData(ToGEMPPlayer(this)->GetSteamHash()));
		engine->ClientCmd_Unrestricted(cmd);
		DevMsg("Sent skin unlock code of %s", cmd);
		m_bSentUnlockCode = true;
	}

	// Check if we changed weapons to reset zoom
	// This catches cases when WeaponSwitch is only called on the server
	if ( GetActiveWeapon() != m_hActiveWeaponCache )
	{
		m_hActiveWeaponCache = GetActiveWeapon();
	}
}

#include "c_npc_gebase.h"

C_GEPlayer* ToGEPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	if ( pEntity->IsNPC() )
	{
#ifdef _DEBUG
		return dynamic_cast<C_GEPlayer*>( ((C_NPC_GEBase*)pEntity)->GetBotPlayer() );
#else
		return static_cast<C_GEPlayer*>( ((C_NPC_GEBase*)pEntity)->GetBotPlayer() );
#endif
	}
	else if ( pEntity->IsPlayer() )
	{
#ifdef _DEBUG
		return dynamic_cast<C_GEPlayer*>( pEntity );
#else
		return static_cast<C_GEPlayer*>( pEntity );
#endif
	}

	return NULL;
}
