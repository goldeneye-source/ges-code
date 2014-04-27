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

	RecvPropBool( RECVINFO( m_bInAimMode ) ),

	RecvPropEHandle( RECVINFO( m_hHat ) ),
	RecvPropInt( RECVINFO( m_takedamage ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_GEPlayer )
	DEFINE_PRED_FIELD( m_bInAimMode, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

extern ConVar v_viewmodel_fov;

static ConVar ge_fp_ragdoll ( "ge_fp_ragdoll", "1", FCVAR_ARCHIVE, "Allow first person ragdolls" );
static ConVar ge_fp_ragdoll_auto ( "ge_fp_ragdoll_auto", "1", FCVAR_ARCHIVE, "Autoswitch to ragdoll thirdperson-view when necessary" );
ConVar cl_ge_nohitothersound( "cl_ge_nohitsound", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Disable the sound that plays when you damage another player" );

C_GEPlayer::C_GEPlayer()
{
	ListenForGameEvent( "specialmusic_setup" );

	m_bInSpecialMusic = false;
	m_flEndSpecialMusic = 0.0f;
	m_iMaxArmor = MAX_ARMOR;
	m_iMaxHealth = MAX_HEALTH;

	m_iAimModeState = AIM_NONE;

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

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

bool C_GEPlayer::CanSetSoundMixer( void )
{
	if ( m_bInSpecialMusic )
		return false;

	return BaseClass::CanSetSoundMixer();
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
		CheckAimMode();

		// Check if we should draw our hat
		if ( m_hHat.Get() )
		{
			if ( IsAlive() )
				m_hHat.Get()->AddEffects( EF_NODRAW );
			else
				m_hHat.Get()->RemoveEffects( EF_NODRAW );
		}
		
		// Adjust the weapon's FOV based on our own FOV
		if ( GetFOV() <= 35.0f )
			v_viewmodel_fov.SetValue( 70.0f );
		else if ( v_viewmodel_fov.GetFloat() != 54.0 )
			v_viewmodel_fov.SetValue( 54.0f );
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
	// This catches cases when WeaponSwitch is only called on the server
	if ( GetActiveWeapon() != m_hActiveWeaponCache )
	{
		m_hActiveWeaponCache = GetActiveWeapon();
		ResetAimMode();
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
