///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_weapon.cpp
// Description:
//      All Goldeneye Weapons derive from this class.
//
// Created On: 2/25/2008 1800
// Created By: Jonathan White <killermonkey01>
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "npcevent.h"

#ifdef CLIENT_DLL
	// This is for the override to FireEvent
	#include "eventlist.h"
	#include "cl_animevent.h"
	#include "view.h"
	#include "c_te_legacytempents.h"

	#include "c_ge_player.h"
	#include "ge_screeneffects.h"
#else
	#include "ge_player.h"
	#include "npc_gebase.h"
	#include "particle_parse.h"
	#include "ge_tokenmanager.h"
#endif

#include "gemp_gamerules.h"
#include "ge_weapon.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ----------------------------------------------------------------------------- //
// Global functions.
// ----------------------------------------------------------------------------- //

#ifdef CLIENT_DLL
ConVar cl_ge_nomuzzlesmoke("cl_ge_nomuzzlesmoke", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
#endif

// ----------------------------------------------------------------------------- //
// CGEWeapon tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeapon, DT_GEWeapon )

BEGIN_NETWORK_TABLE( CGEWeapon, DT_GEWeapon )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bSilenced ) ),
	RecvPropFloat( RECVINFO( m_flAccuracyPenalty ) ),
	RecvPropFloat( RECVINFO( m_flCoolDownTime) ),
	RecvPropBool( RECVINFO(m_bEnableGlow) ),
	RecvPropInt( RECVINFO(m_GlowColor), 0, RecvProxy_IntToColor32 ),
	RecvPropFloat( RECVINFO(m_GlowDist) ),
#else
	SendPropBool( SENDINFO( m_bSilenced ) ),
	SendPropFloat( SENDINFO( m_flAccuracyPenalty ) ),
	SendPropFloat( SENDINFO( m_flCoolDownTime) ),
	SendPropBool( SENDINFO(m_bEnableGlow) ),
	SendPropInt( SENDINFO(m_GlowColor), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
	SendPropFloat( SENDINFO(m_GlowDist) ),
#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )

#include "globalstate.h"
//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CGEWeapon )

	DEFINE_FIELD( m_bSilenced,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLowered,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flRaiseTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flHolsterTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flAccuracyPenalty,	FIELD_TIME ),
	DEFINE_FIELD( m_flCoolDownTime,		FIELD_TIME ),

END_DATADESC()
#endif

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CGEWeapon )
	DEFINE_PRED_FIELD( m_bSilenced, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCoolDownTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

// ----------------------------------------------------------------------------- //
// CWeaponCSBase implementation. 
// ----------------------------------------------------------------------------- //
CGEWeapon::CGEWeapon()
{
	m_flHolsterTime = 0.0f;
	m_bSilenced = false;
	m_bIsAlwaysSilent = false;
	m_flAccuracyPenalty = m_flCoolDownTime = 0.0f;
	// Our weapons are not water proof by default, but the alt fire is
	m_bFiresUnderwater = false;
	m_bAltFiresUnderwater = true;
	m_bEnableGlow = false;

	m_flSmokeRate = m_flLastShotTime = 0;
	m_iShotsFired = 0;

#ifdef GAME_DLL
	m_flDeployTime = 0.0f;

	color32 col32 = { 255, 255, 255, 100 };
	m_GlowColor.Set( col32 );
	m_GlowDist.Set( 250.0f );
#else
	m_bClientGlow = false;
	m_pEntGlowEffect = (CEntGlowEffect*)g_pScreenSpaceEffects->GetScreenSpaceEffect("ge_entglow");
#endif
}

#ifdef GAME_DLL

CGEWeapon::~CGEWeapon()
{
}

void CGEWeapon::Spawn()
{
	AddSpawnFlags(SF_NORESPAWN);
	// So NPC's will "see" us
	AddFlag( FL_OBJECT );

	BaseClass::Spawn();

	m_vOriginalSpawnOrigin = GetAbsOrigin();
	m_vOriginalSpawnAngles = GetAbsAngles();

	// Notify the token manager we are arriving
	GEMPRules()->GetTokenManager()->OnTokenSpawned( this );
}

void CGEWeapon::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
	// Notify the token manager we are going away
	if ( GEMPRules() && GEMPRules()->GetTokenManager() )
		GEMPRules()->GetTokenManager()->OnTokenRemoved( this );
}

#endif //GAME_DLL

void CGEWeapon::Precache( void )
{
	BaseClass::Precache();

	// Best place to initialize this on client/server...
	m_flSmokeRate = GetClickFireRate() * GetMaxClip1() * 0.4f;

	PrecacheParticleSystem( "tracer_standard" );
	PrecacheParticleSystem( "muzzle_smoke" );
}

void CGEWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

#ifdef GAME_DLL
	GEMPRules()->GetTokenManager()->OnTokenPicked( this, ToGEPlayer( pOwner ) );

	// Disable Glowing (but preserve our set color)
	SetEnableGlow( false );
#endif

	// Fill this bad boy up with ammo if we have any for it to use!
	FinishReload();
}

void CGEWeapon::Drop( const Vector &vecVelocity )
{
#ifdef GAME_DLL
	// Call this first so we can capture who owned the entity before the drop
	GEMPRules()->GetTokenManager()->OnTokenDropped( this, ToGEPlayer(GetOwner()) );

	SetEnableGlow( m_bServerGlow );
#endif

	BaseClass::Drop( vecVelocity );
}

void CGEWeapon::SetPickupTouch( void )
{
#ifndef CLIENT_DLL
	SetTouch(&CBaseCombatWeapon::DefaultTouch);

	if ( gpGlobals->maxClients > 1 )
	{
		if ( GetSpawnFlags() & SF_NORESPAWN )
		{
			SetThink( &CBaseEntity::SUB_Remove );
			float delay = 30.0f;
			GEMPRules()->GetTokenManager()->GetTokenRespawnDelay( GetClassname(), &delay );

			SetNextThink( gpGlobals->curtime + delay );
		}
	}
#endif
}

bool CGEWeapon::PlayEmptySound()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );
	
	return 0;
}

void CGEWeapon::RecordShotFired(int count /*=1*/)
{
	m_iShotsFired += count;
	m_flLastShotTime = gpGlobals->curtime;
}

void CGEWeapon::PrimaryAttack(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if(!pPlayer)
		return;

	//This stops silent firing...
	if(pPlayer->m_nButtons & IN_RELOAD)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
		return;
	}

	// Send the animation event to the client/server
	//ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	RecordShotFired();

	BaseClass::PrimaryAttack();
}

bool CGEWeapon::Reload( void )
{
	if ( m_flNextPrimaryAttack > gpGlobals->curtime || m_flNextSecondaryAttack > gpGlobals->curtime )
		return false;

	bool bRet = false;

	if ( IsSilenced() )
		bRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_SILENCED );
	else
		bRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );

	if ( bRet && GetOwner() && GetOwner()->IsPlayer() )
	{
		CGEPlayer *pPlayer = ToGEPlayer( GetOwner() );

	#ifdef CLIENT_DLL
		IGameEvent *event = gameeventmanager->CreateEvent( "weapon_event" );
		if ( event )
		{
			event->SetInt( "userid", ToBasePlayer(GetOwner())->GetUserID() );
			event->SetInt( "weaponid", GetWeaponID() );
			event->SetInt( "eventid", WEAPON_EVENT_RELOAD );
			gameeventmanager->FireEventClientSide( event );
		}
	#endif
		// Reset our aim mode
		pPlayer->ResetAimMode( true );

		m_iShotsFired = 0;
		//pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
	}

	return bRet;
}

void CGEWeapon::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CGEWeapon::ToggleSilencer( bool doanim /* = true */ )
{
	if ( CanBeSilenced() )
	{
		if ( m_bSilenced && doanim )
		{
			SendWeaponAnim( ACT_VM_DETACH_SILENCER );
			WeaponSound(SPECIAL1);
		}
		else if ( doanim )
		{
			SendWeaponAnim( ACT_VM_ATTACH_SILENCER );
			WeaponSound(SPECIAL2);
		}

		m_bSilenced = !m_bSilenced;
	}
}

int CGEWeapon::GetTracerAttachment( void )
{
	return BaseClass::GetTracerAttachment();
}

#ifdef CLIENT_DLL

#define GE_BOB_W		0.65f
#define GE_BOB_H		0.3f
#define GE_BOB_TIME		1.25f

extern float	g_lateralBob;
extern float	g_verticalBob;

float CGEWeapon::CalcViewmodelBob( void )
{
	static	float bobtime;
	static	float lastbobtime;
	float	cycle;
	
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	if ( !gpGlobals->frametime || player == NULL )
		return 0;

	// Find the speed ratio vs max speed
	float speedrat = player->GetLocalVelocity().Length2D() / (float)GE_NORM_SPEED;
	speedrat = clamp( speedrat, 0, 1.2f );

	// Calculate the width/height based on our speed ratio
	float w = GE_BOB_W * speedrat;
	float h = GE_BOB_H * speedrat;
	
	// Find our bobtime
	bobtime += gpGlobals->curtime - lastbobtime;
	lastbobtime = gpGlobals->curtime;

	cycle = bobtime / GE_BOB_TIME;
	
	g_lateralBob = w * sin( M_TWOPI * cycle );
	g_verticalBob = h * sin ( 2.0f * M_TWOPI * cycle );
	
	return 0;
}

void CGEWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	Vector	forward, right, up;
	AngleVectors( angles, &forward, &right, &up );

	CalcViewmodelBob();

	// Apply bob
	VectorMA( origin, g_verticalBob, up, origin );
	VectorMA( origin, g_lateralBob, right, origin );
	VectorMA( origin, g_lateralBob * 0.5f, forward, origin );
	
	// bob the angles
	angles[ ROLL ]	+= g_lateralBob;
	angles[ PITCH ]	-= g_verticalBob * 0.8f;
	angles[ YAW ]	-= g_lateralBob  * 0.6f;
}

void CGEWeapon::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	color32 col = m_GlowColor.Get();
	if ( m_bClientGlow != m_bEnableGlow )
	{
		if ( m_bEnableGlow )
		{
			m_pEntGlowEffect->RegisterEnt( this, Color(col.r, col.g, col.b, col.a), m_GlowDist.Get() );
		}
		else
		{
			m_pEntGlowEffect->DeregisterEnt( this );
		}

		m_bClientGlow = m_bEnableGlow;
	}
	else
	{
		m_pEntGlowEffect->SetEntColor( this, Color(col.r, col.g, col.b, col.a) );
		m_pEntGlowEffect->SetEntMaxGlowDist( this, m_GlowDist.Get() );
	}
}

void CGEWeapon::UpdateVisibility( void )
{
	// If we are glowing always make us visible so that we can draw the glow event
	if ( m_bClientGlow )
		AddToLeafSystem();
	else
		BaseClass::UpdateVisibility();
}

#endif

Activity CGEWeapon::GetPrimaryAttackActivity( void )
{
	if ( IsSilenced() )
		return ACT_VM_PRIMARYATTACK_SILENCED;
	else 
		return ACT_VM_PRIMARYATTACK;
}

Activity CGEWeapon::GetDrawActivity( void )
{
	if ( IsSilenced() )
		return ACT_VM_DRAW_SILENCED;
	else
		return ACT_VM_DRAW;
}

void CGEWeapon::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		if ( IsSilenced() )
			SendWeaponAnim( ACT_VM_IDLE_SILENCED );
		else
			SendWeaponAnim( ACT_VM_IDLE );
	}
}

#ifdef GAME_DLL

void CGEWeapon::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	if ( gpGlobals->curtime < m_flNextPrimaryAttack )
		DevWarning( 2, "NPC attempted fire too early (%0.2f) with %s!\n", m_flNextPrimaryAttack - gpGlobals->curtime, GetClassname() );

	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	WeaponSound( SINGLE_NPC );
	pOperator->DoMuzzleFlash();
	// Reduce our ammo count
	m_iClip1 = m_iClip1 - 1;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

		// NOTE: CBaseCombatCharacter::FindMissTarget( void ) is jacked up
	}

	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );
}

void CGEWeapon::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_AR2:
			FireNPCPrimaryAttack( pOperator, false );
			break;
			
		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

void CGEWeapon::DefaultTouch( CBaseEntity *pOther )
{
	// Handle NPC's touching us
	if ( pOther->IsNPC() )
	{
		CNPC_GEBase *pNPC = (CNPC_GEBase*) pOther;

		if( HasSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP) )
			return;

		if ( pNPC->BumpWeapon(this) )
			OnPickedUp( pNPC );
	}
	else
	{
		BaseClass::DefaultTouch( pOther );
	}
}

void CGEWeapon::SetupGlow( bool state, Color glowColor /*=Color(255,255,255)*/, float glowDist /*=250.0f*/ )
{
	m_bServerGlow = state;
	color32 col32 = { glowColor.r(), glowColor.g(), glowColor.b(), glowColor.a() };
	m_GlowColor.Set( col32 );
	m_GlowDist.Set( glowDist );

	if ( !GetOwner() || (GetOwner() && !GetOwner()->IsPlayer()) )
		SetEnableGlow( state );
}

void CGEWeapon::SetEnableGlow( bool state )
{
	m_bEnableGlow = state;
}

#endif

const Vector& CGEWeapon::GetBulletSpread( void )
{
    // Static due to return value...
	static Vector result;
	Vector minSpread, maxSpread;

	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner && pOwner->IsPlayer() )
	{
		CGEPlayer *pPlayer = ToGEPlayer( pOwner );

		// Give penalty for being in the air (jumping/falling)
		// (ladder check: pPlayer->GetMoveType() == MOVETYPE_LADDER)
		if ( pPlayer->GetGroundEntity() == NULL )
			minSpread = GetGEWpnData().m_vecSpread * 1.2f;
		else if( pPlayer->IsInAimMode() )
			minSpread = GetGEWpnData().m_vecSpreadSighted;
		else 
			minSpread = GetGEWpnData().m_vecSpread;
	}
	else
	{
		minSpread = GetGEWpnData().m_vecSpread;
	}

	// Make sure we enforce the cool down
	if ( gpGlobals->curtime >= m_flCoolDownTime )
		maxSpread = minSpread;
	else
		maxSpread = minSpread + Vector(1,1,0) * ( GetRecoil()*(m_flCoolDownTime - gpGlobals->curtime) / (3.0f * GetFireRate()) );

	// Update the cool down time
	if ( (gpGlobals->curtime - m_flCoolDownTime) <= (2.0f*GetFireRate()) )
		m_flCoolDownTime = gpGlobals->curtime + 2.0f*GetFireRate();
	else
		m_flCoolDownTime = gpGlobals->curtime + 4.0f*GetFireRate();
	
	// Map our penalty time from 0-MaxPenalty  to  0-1
	float ramp = RemapValClamped( m_flAccuracyPenalty, 0.0f, GetMaximumPenaltyTime(), 0.0f, 1.0f ); 

	// We lerp from very accurate to inaccurate over time
	VectorLerp( minSpread, maxSpread, ramp, result );
	
	return result;
}

void CGEWeapon::UpdatePenaltyTime( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Check our penalty time decay
	if ( (pOwner->m_nButtons & IN_ATTACK) == false  && m_flAccuracyPenalty != 0 )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = max( m_flAccuracyPenalty, 0.0f );
	}
}

bool CGEWeapon::HasAmmo( void )
{
	// Weapons with no ammo types can always be selected
	if ( m_iPrimaryAmmoType == -1 && m_iSecondaryAmmoType == -1  )
		return true;
	if ( GetWeaponFlags() & ITEM_FLAG_SELECTONEMPTY )
		return true;

	CBaseCombatCharacter *owner = GetOwner();
	if ( !owner )
		return false;

	return ( m_iClip1 > 0 || owner->GetAmmoCount( m_iPrimaryAmmoType ) || m_iClip2 > 0 || owner->GetAmmoCount( m_iSecondaryAmmoType ) );
}

bool CGEWeapon::HasShotsLeft( void )
{
	if ( m_iPrimaryAmmoType == -1 && m_iSecondaryAmmoType == -1  )
		return true;

	CBaseCombatCharacter *owner = GetOwner();
	if ( !owner )
		return false;

	return (UsesClipsForAmmo1() && m_iClip1 > 0) || owner->GetAmmoCount( m_iPrimaryAmmoType );
}

bool CGEWeapon::IsWeaponVisible( void )
{
	if ( GetOwner() && GetOwner()->IsNPC() )
		return !IsEffectActive( EF_NODRAW );
	else
		return BaseClass::IsWeaponVisible();
}

void CGEWeapon::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();
	UpdatePenaltyTime();
}

void CGEWeapon::ItemPostFrame( void )
{
	// Reset shots fired if we go past our reset time (do this before any possible call to Primary Attack)
	if ( m_iShotsFired && (gpGlobals->curtime - m_flSmokeRate) > m_flLastShotTime )
		m_iShotsFired = 0;

	BaseClass::ItemPostFrame();

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	if ( pOwner->m_afButtonPressed & IN_ATTACK )
	{
	#ifdef CLIENT_DLL
		if ( pOwner->IsLocalPlayer() && pOwner->GetViewModel() )
			pOwner->GetViewModel()->ParticleProp()->StopParticlesNamed("muzzle_smoke");
	#else
		StopParticleEffects( this );
	#endif
	}
	else if ( (pOwner->m_nButtons & IN_ATTACK) == false && m_iShotsFired > 0 )
	{
		float shotlimit = 0.4f*GetMaxClip1();
	#ifdef CLIENT_DLL
		bool doSmoke = !cl_ge_nomuzzlesmoke.GetBool() && m_iShotsFired > shotlimit;
	#else
		bool doSmoke = m_iShotsFired > shotlimit;
	#endif

		if ( doSmoke && !(IsSilenced() || IsAlwaysSilenced()) )
		{
			float prob = RemapValClamped( m_iShotsFired, shotlimit, GetMaxClip1(), 0.4f, 0.05f );
			if ( GERandom<float>(1.0f) > prob )
			{
			#ifdef CLIENT_DLL
				if ( pOwner->IsLocalPlayer() && pOwner->GetViewModel() )
					pOwner->GetViewModel()->ParticleProp()->Create( "muzzle_smoke", PATTACH_POINT_FOLLOW, "muzzle" );
			#else
					DispatchParticleEffect( "muzzle_smoke", PATTACH_POINT_FOLLOW, this, "muzzle" );
			#endif
			}

			m_iShotsFired = 0;
		}
	}
}

bool CGEWeapon::Deploy( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return false;

	if ( BaseClass::Deploy() )
	{
#ifdef GAME_DLL
		m_flDeployTime = gpGlobals->curtime;
#else
		if ( pOwner && pOwner->IsPlayer() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "weapon_event" );
			if ( event )
			{
				event->SetInt( "userid", ToBasePlayer(pOwner)->GetUserID() );
				event->SetInt( "weaponid", GetWeaponID() );
				event->SetInt( "eventid", WEAPON_EVENT_DRAW );
				gameeventmanager->FireEventClientSide( event );
			}
		}
#endif
		//if ( pOwner && pOwner->IsPlayer() )
		//	ToGEPlayer( pOwner )->DoAnimationEvent( PLAYERANIMEVENT_GES_DRAW );
		
		return true;
	}

	return false;
}

bool CGEWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return false;

	if ( BaseClass::Holster( pSwitchingTo ) )
	{
	#ifdef CLIENT_DLL
		if ( pOwner->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pOwner );

			// Stop emitting all particles!
			CBaseViewModel *pVM = pPlayer->GetViewModel();
			if ( pVM && pVM->ParticleProp() )
				pVM->ParticleProp()->StopEmission();

			IGameEvent *event = gameeventmanager->CreateEvent( "weapon_event" );
			if ( event )
			{
				event->SetInt( "userid", pPlayer->GetUserID() );
				event->SetInt( "weaponid", GetWeaponID() );
				event->SetInt( "eventid", WEAPON_EVENT_HOLSTER );
				gameeventmanager->FireEventClientSide( event );
			}
		}
	#else
		StopParticleEffects( this );
	#endif
		
//		if ( pOwner->IsPlayer() )
//		{
//			CGEPlayer *pPlayer = ToGEPlayer( pOwner );
//			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_GES_HOLSTER );
//		}

		return true;
	}

	return false;
}

void CGEWeapon::SetSkin( int skin )
{
#ifdef GAME_DLL
	m_nSkin = skin;
#endif

	CGEPlayer *pGEPlayer = ToGEPlayer( GetOwner() );
	if ( !pGEPlayer )
		return;

	CBaseViewModel *vm = pGEPlayer->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
		return;

	vm->m_nSkin = m_nSkin;
}

void CGEWeapon::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	CBaseEntity *pOwner = GetOwner();

	if ( pOwner == NULL )
	{
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		return;
	}

	Vector vNewSrc = vecTracerSrc;
	int iEntIndex = pOwner->entindex();

	if ( g_pGameRules->IsMultiplayer() )
	{
		iEntIndex = entindex();
	}

	int iAttachment = GetTracerAttachment();

	switch ( iTracerType )
	{
	case TRACER_LINE:
		UTIL_ParticleTracer( "tracer_standard", vNewSrc, tr.endpos, iEntIndex, iAttachment, false );
		break;

	case TRACER_LINE_AND_WHIZ:
		UTIL_ParticleTracer( "tracer_standard", vNewSrc, tr.endpos, iEntIndex, iAttachment, true );
		break;
	}
}

float CGEWeapon::GetZoomOffset()
{
	return GetGEWpnData().m_iZoomOffset;
}

// GE Weapon Data accessor functions
const CGEWeaponInfo &CGEWeapon::GetGEWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CGEWeaponInfo *pGEInfo;

	#ifdef _DEBUG
		pGEInfo = dynamic_cast< const CGEWeaponInfo* >( pWeaponInfo );
		Assert( pGEInfo );
	#else
		pGEInfo = static_cast< const CGEWeaponInfo* >( pWeaponInfo );
	#endif

	return *pGEInfo;
}
float CGEWeapon::GetFireRate( void )
{
	return GetGEWpnData().m_flRateOfFire;
}
float CGEWeapon::GetClickFireRate( void )
{
	return GetGEWpnData().m_flClickRateOfFire;
}
float CGEWeapon::GetFireDelay( void )
{
	return GetGEWpnData().m_flFireDelay;
}
// END Accessor functions
