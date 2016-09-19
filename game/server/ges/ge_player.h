///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_player.h
// Description:
//      see ge_player.cpp
//
// Created On: 23 Feb 08, 16:35
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_PLAYER_H
#define GE_PLAYER_H
#pragma once

class CGEPlayer;
class CGEWeapon;

#include "basemultiplayerplayer.h"
#include "hl2_playerlocaldata.h"
#include "hl2mp_player.h"
#include "soundenvelope.h"
#include "ge_player_shared.h"
#include "ge_weapon.h"
#include "ge_gamerules.h"
#include "utldict.h"
#include "shareddefs.h"

//=============================================================================
// >> CGEPlayer
//=============================================================================
class CGEPlayer : public CHL2MP_Player
{
public:
	DECLARE_CLASS(CGEPlayer, CHL2MP_Player);

	CGEPlayer();
	~CGEPlayer(void);


	static CGEPlayer *CreatePlayer(const char *className, edict_t *ed)
	{
		CGEPlayer::s_PlayerEdict = ed;

		if (Q_strcmp(className, "player") == 0)
			return (CGEPlayer*)CreateEntityByName("mp_player");
		else
			return (CGEPlayer*)CreateEntityByName(className);
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache(void);
	virtual void PrecacheFootStepSounds(void) { };

	virtual void InitialSpawn(void);
	virtual void Spawn();

	// -------------------------------------------------
	// GES Functions
	// -------------------------------------------------

	//This will return true if the player is 'fully' in AimMode
	void ResetAimMode(bool forced = false);
	bool IsInAimMode();
	bool StartedAimMode()					{ return (m_flFullZoomTime > 0); }
	bool IsRadarCloaked()					{ return m_bInSpawnInvul; }
	bool AddArmor( int amount );
	bool CheckInPVS(CBaseEntity *pEnt);

	virtual void KnockOffHat( bool bRemove = false, const CTakeDamageInfo *dmg = NULL );
	// Main way to assign hats to characters.  Uses model specific hats.
	virtual void GiveHat( void );
	// Method of assigning whatever hat you want to a player.
	virtual void SpawnHat( const char* hatModel, bool canBeRemoved = true );

	void HideBloodScreen( void );
	void ShowBloodScreen( void );

	// Functions for Python
	void  SetSpeedMultiplier(float mult);
	float GetSpeedMultiplier()				{ return m_flSpeedMultiplier; }
	void  SetDamageMultiplier( float mult )	{ m_flDamageMultiplier = clamp( mult, 0, 200.0f ); }
	float GetDamageMultiplier()				{ return m_flDamageMultiplier; }
	void  SetMaxArmor( int armor )			{ m_iMaxArmor = armor; }
	int   GetMaxArmor( void )				{ return m_iMaxArmor; }
	int   GetScoreBoardColor( void )		{ return m_iScoreBoardColor; }
	void  SetScoreBoardColor( int color )	{ m_iScoreBoardColor = color; }

	// These are accessors to the Accuracy and Efficiency statistics for this player (set by GEStats)
	int  GetFavoriteWeapon( void )		{ return m_iFavoriteWeapon; }
	void SetFavoriteWeapon( int wep )	{ m_iFavoriteWeapon = wep; }

	// Custom damage tracking function
	virtual void Event_DamagedOther( CGEPlayer *pOther, int dmgTaken, const CTakeDamageInfo &inputInfo );
	virtual void PlayHitsound(int dmgTaken, int dmgtype);
	virtual void PlayKillsound();

	// -------------------------------------------------

	// Redfine this to ensure when we call this function we set the weapons to loadup on the right VM
	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = GE_RIGHT_HAND );
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );

	virtual void GiveAllItems( void );
	virtual int  GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound );
	
	virtual int  SpawnArmorValue( void ) const { return 0; }

    virtual void PreThink( void );
	virtual void PostThink( void );

	// Setup the hint system so we can show button hints
	virtual CHintSystem	 *Hints( void ) { return m_pHints; }

	// Used to setup invul time
	virtual int  OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int	 OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator = NULL );
	
	// Custom Pain Sounds
	virtual void OnDamagedByExplosion( const CTakeDamageInfo &info ) { }
	virtual void DeathSound( const CTakeDamageInfo &info ) { }
	virtual void HurtSound( const CTakeDamageInfo &info );
	
	// Move the viewmodel closer to us if we are crouching
	virtual void  CalcViewModelView( const Vector& eyeOrigin, const QAngle& eyeAngles);
	virtual float GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );

	virtual void	RemoveAmmo(int iCount, int iAmmoIndex);
	virtual void	FireBullets( const FireBulletsInfo_t &info );
	virtual Vector	GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );

	virtual void FinishClientPutInServer();
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void DropAllTokens();
	virtual void DropTopWeapons();

	virtual void SetPlayerModel( void );
	virtual void SetPlayerModel( const char* szCharName, int iCharSkin = 0, bool bNoSelOverride = false );

	virtual bool StartObserverMode( int mode );

	// Functions for achievements
	virtual int			GetCharIndex( void ) { return m_iCharIndex; };
	virtual const char* GetCharIdent( void );

	void HintMessage( const char *pMessage, float duration = 6.0f ) { if (Hints()) Hints()->HintMessage( pMessage, duration ); }

	virtual bool IsMPPlayer()=0;
	virtual bool IsSPPlayer()=0;
	virtual bool IsBotPlayer() const { return false; }

	virtual void CheatImpulseCommands( int iImpulse );

	virtual void GiveNamedWeapon(const char* ident, int ammoCount, bool stripAmmo = false );
	virtual void StripAllWeapons();

	virtual CBaseEntity* GetLastAttacker(bool onlyrecent = true);
	virtual void SetLastAttacker(CBaseEntity* lastAttacker) { m_pLastAttacker = lastAttacker; }

	void CheckAimMode(void);

protected:
	// Purely GE Functions & Variables
	bool ShouldRunRateLimitedCommand( const CCommand &args );

	void NotifyPickup( const char *classname, int type );

	void StartInvul( float time );
	void StopInvul( void );

	int CalcInvul(int damage, CGEPlayer *pAttacker, int weapid);

	void SetCharIndex(int index){m_iCharIndex = index;}

	CHintSystem *m_pHints;

	int m_iRoundScore;
	int m_iFavoriteWeapon;
	int m_iScoreBoardColor;
	int m_iCharIndex;
	int m_iSkinIndex;
	int m_bRemovableHat;

	float m_flDamageMultiplier;
	float m_flSpeedMultiplier;

	int m_iFrameDamageOutput;
	int m_iFrameDamageOutputType;
	bool m_bKilledOtherThisFrame;

	// Invulnerability variables
	float		m_flEndInvulTime;
	bool		m_bInSpawnInvul;
	int			m_iViewPunchScale;
	int			m_iDmgTakenThisFrame;
	Vector		m_vDmgForceThisFrame;
	int			m_iAttackList [16];
	float		m_iAttackListTimes [16];

	// Explosion invulnerability variables
	float		m_flEndExpDmgTime;
	int			m_iExpDmgTakenThisInterval;
	Vector		m_vExpDmgForceThisFrame;

	// Kill credit variables, these are important for tracking who gets credit for suicides and other things.
	CBaseEntity *m_pLastAttacker;
	float		m_flLastAttackedTime;
	int			m_iLastAttackedDamage;

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float,int>	m_RateLimitLastCommandTimes;

	CNetworkVar( int,	m_iMaxArmor );

	// Let's us know when we are officially in aim mode
	CNetworkVar(float, m_flFullZoomTime);

	CNetworkHandle( CBaseCombatWeapon,	m_hActiveLeftWeapon );
	CNetworkHandle( CBaseEntity,		m_hHat );

private:

	byte m_iPVS[MAX_MAP_CLUSTERS / 8];
};

CGEPlayer *ToGEPlayer( CBaseEntity *pEntity );

/*
* Just a quick macro to not recode this a lot...
*/
#define FOR_EACH_PLAYER( var ) \
	for( int _iter=1; _iter <= gpGlobals->maxClients; ++_iter ) { \
		CGEPlayer *var = ToGEPlayer( UTIL_PlayerByIndex( _iter ) ); \
		if ( var == NULL || FNullEnt( var->edict() ) || !var->IsPlayer() ) \
			continue;

#define END_OF_PLAYER_LOOP() }

#endif //GE_PLAYER_H

