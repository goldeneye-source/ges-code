///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai_sense.cpp
//   Description :
//      [TODO: Write the purpose of ge_ai_sense.cpp.]
//
//   Created On: 3/8/2009 12:01:20 PM
//   Created By:  <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_ai_sense.h"
#include "ai_senses.h"
#include "ge_player.h"
#include "soundent.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float AI_NPC_SEARCH_TIME = .25;
const float AI_HIGH_PRIORITY_SEARCH_TIME = 0.15;
const float AI_MISC_SEARCH_TIME = .25;

CGEAISense::CGEAISense(CGEAIHandle* baseAi) : CGEAIHelper(baseAi)
{
	m_fLookDist = 2048,
	m_fLastLookDist = -1;
	m_fTimeLastLook = -1;

	m_fTimeLastLookHighPriority = -1 ;
	m_fTimeLastLookNPCs = -1 ;
	m_fTimeLastLookMisc = -1 ;

	m_SeenArrays[0] = &m_SeenHighPriority;
	m_SeenArrays[1] = &m_SeenNPCs;
	m_SeenArrays[2] = &m_SeenMisc;
}

CGEAISense::~CGEAISense()
{

}

bool CGEAISense::CanHearSound( CSound *pSound )
{
	if ( pSound->m_hOwner.Get() == GetPlayer() )
		return false;

	// @TODO (toml 10-18-02): what about player sounds and notarget?
	float flHearDistanceSq = pSound->Volume() * GetAIHandle()->GetHearingSensitivity();
	flHearDistanceSq *= flHearDistanceSq;
	if( pSound->GetSoundOrigin().DistToSqr( GetPlayer()->EarPosition() ) <= flHearDistanceSq )
	{
		return CanHearSound( pSound );
	}

	return false;
}


//-----------------------------------------------------------------------------
// Listen - npcs dig through the active sound list for
// any sounds that may interest them. (smells, too!)

void CGEAISense::Listen( void )
{
	m_vSoundList.Purge();

	int iSoundMask = GetAIHandle()->GetSoundInterests();
	
	if ( iSoundMask != SOUND_NONE)
	{
		int	iSound = CSoundEnt::ActiveList();
		
		while ( iSound != SOUNDLIST_EMPTY )
		{
			CSound *pCurrentSound = CSoundEnt::SoundPointerForIndex( iSound );

			if ( pCurrentSound	&& (iSoundMask & pCurrentSound->SoundType()) && CanHearSound( pCurrentSound ) )
			{
	 			// the npc cares about this sound, and it's close enough to hear.
				m_vSoundList.AddToTail(iSound);
			}

			iSound = pCurrentSound->NextSound();
		}
	}
}

//-----------------------------------------------------------------------------

bool CGEAISense::ShouldSeeEntity( CBaseEntity *pSightEnt )
{
	if ( pSightEnt == GetPlayer() || !pSightEnt->IsAlive() )
		return false;

	if ( pSightEnt->IsPlayer() && ( pSightEnt->GetFlags() & FL_NOTARGET ) )
		return false;

	// don't notice anyone waiting to be seen by the player
	if ( pSightEnt->m_spawnflags & SF_NPC_WAIT_TILL_SEEN )
		return false;

	//if ( !pSightEnt->CanBeSeenBy( GetPlayer() ) )
	//	return false;
	//
	//if ( !CanSeeEntity( pSightEnt, true ) )
	//	return false;

	return true;
}

//-----------------------------------------------------------------------------

bool CGEAISense::CanSeeEntity( CBaseEntity *pSightEnt )
{
	return ( GetPlayer()->FInViewCone( pSightEnt ) && GetPlayer()->FVisible( pSightEnt ) );
}

//-----------------------------------------------------------------------------

bool CGEAISense::DidSeeEntity( CBaseEntity *pSightEnt )
{
	for (int x=0; x<GetEntityCount(); x++)
	{
		if ( pSightEnt == GetEntity(x) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

void CGEAISense::NoteSeenEntity( CBaseEntity *pSightEnt )
{
	pSightEnt->m_pLink = GetPlayer()->m_pLink;
	GetPlayer()->m_pLink = pSightEnt;
}

//-----------------------------------------------------------------------------

bool CGEAISense::SeeEntity( CBaseEntity *pSightEnt )
{
	// insert at the head of my sight list
	NoteSeenEntity( pSightEnt );
	return true;
}

//-----------------------------------------------------------------------------

void CGEAISense::BeginGather()
{
	// clear my sight list
	GetPlayer()->m_pLink = NULL;
}

//-----------------------------------------------------------------------------

void CGEAISense::EndGather( int nSeen, CUtlVector<EHANDLE> *pResult )
{
	pResult->SetCount( nSeen );
	if ( nSeen )
	{
		CBaseEntity *pCurrent = GetPlayer()->m_pLink;
		for (int i = 0; i < nSeen; i++ )
		{
			Assert( pCurrent );
			(*pResult)[i].Set( pCurrent );
			pCurrent = pCurrent->m_pLink;
		}
		GetPlayer()->m_pLink = NULL;
	}
}

//-----------------------------------------------------------------------------
// Look - Base class npc function to find enemies or 
// food by sight. iDistance is distance ( in units ) that the 
// npc can see.
//
// Sets the sight bits of the m_afConditions mask to indicate
// which types of entities were sighted.
// Function also sets the Looker's m_pLink 
// to the head of a link list that contains all visible ents.
// (linked via each ent's m_pLink field)
//

void CGEAISense::Look( int iDistance )
{
	if ( m_fTimeLastLook != gpGlobals->curtime || m_fLastLookDist != iDistance )
	{
		//-----------------------------
		
		LookForHighPriorityEntities( iDistance );
		LookForNPCs( iDistance);
		LookForObjects( iDistance );
		
		//-----------------------------
		
		m_fLastLookDist = iDistance;
		m_fTimeLastLook = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------

bool CGEAISense::Look( CBaseEntity *pSightEnt )
{
	if ( ShouldSeeEntity( pSightEnt ) && CanSeeEntity( pSightEnt ) )
		return SeeEntity( pSightEnt );

	return false;
}

//-----------------------------------------------------------------------------

int CGEAISense::LookForHighPriorityEntities( int iDistance )
{
	int nSeen = 0;
	if ( gpGlobals->curtime - m_fTimeLastLookHighPriority > AI_HIGH_PRIORITY_SEARCH_TIME )
	{
		m_fTimeLastLookHighPriority = gpGlobals->curtime;
		
		BeginGather();
	
		float distSq = ( iDistance * iDistance );
		const Vector &origin = GetPlayer()->GetAbsOrigin();
		
		// Players
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer )
			{
				if ( origin.DistToSqr(pPlayer->GetAbsOrigin()) < distSq && Look( pPlayer ) )
				{
					nSeen++;
				}
			}
		}
	
		EndGather( nSeen, &m_SeenHighPriority );
    }
    else
    {
    	for ( int i = m_SeenHighPriority.Count() - 1; i >= 0; --i )
    	{
    		if ( m_SeenHighPriority[i].Get() == NULL )
    			m_SeenHighPriority.FastRemove( i );    			
    	}
    	nSeen = m_SeenHighPriority.Count();
    }
	
	return nSeen;
}

//-----------------------------------------------------------------------------

int CGEAISense::LookForNPCs( int iDistance )
{
	return 0;
}

//-----------------------------------------------------------------------------
extern CAI_SensedObjectsManager g_AI_SensedObjectsManager;

int CGEAISense::LookForObjects( int iDistance )
{	
	const int BOX_QUERY_MASK = FL_OBJECT;
	int	nSeen = 0;

	if ( gpGlobals->curtime - m_fTimeLastLookMisc > AI_MISC_SEARCH_TIME )
	{
		m_fTimeLastLookMisc = gpGlobals->curtime;
		
		BeginGather();

		float distSq = ( iDistance * iDistance );
		const Vector &origin = GetPlayer()->GetAbsOrigin();
		int iter;
		CBaseEntity *pEnt = g_AI_SensedObjectsManager.GetFirst( &iter );
		while ( pEnt )
		{
			if ( pEnt->GetFlags() & BOX_QUERY_MASK )
			{
				if ( origin.DistToSqr(pEnt->GetAbsOrigin()) < distSq && Look( pEnt) )
				{
					nSeen++;
				}
			}
			pEnt = g_AI_SensedObjectsManager.GetNext( &iter );
		}
		
		EndGather( nSeen, &m_SeenMisc );
	}
    else
    {
    	for ( int i = m_SeenMisc.Count() - 1; i >= 0; --i )
    	{
    		if ( m_SeenMisc[i].Get() == NULL )
    			m_SeenMisc.FastRemove( i );    			
    	}
    	nSeen = m_SeenMisc.Count();
    }

	return nSeen;
}

//-----------------------------------------------------------------------------

float CGEAISense::GetTimeLastUpdate( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return 0;
	if ( pEntity->IsPlayer() )
		return m_fTimeLastLookHighPriority;
	if ( pEntity->IsNPC() )
		return m_fTimeLastLookNPCs;
	return m_fTimeLastLookMisc;
}

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------

CSound *CGEAISense::GetClosestSound( bool fScent, int validTypes, bool bUsePriority )
{
	float flBestDist = MAX_COORD_RANGE*MAX_COORD_RANGE;// so first nearby sound will become best so far.
	float flDist;
	int iBestPriority = SOUND_PRIORITY_VERY_LOW;
		
	CSound *pResult = NULL;
	Vector earPosition = GetPlayer()->EarPosition();
	
	for (int x=0; x< GetSoundCount(); x++)
	{
		CSound *pCurrent = GetSound(x);

		if ( ( !fScent && pCurrent->FIsSound() ) ||  ( fScent && pCurrent->FIsScent() ) )
		{
			if( pCurrent->IsSoundType( validTypes ) && !GetAIHandle()->ShouldIgnoreSound( pCurrent ) )
			{
				if( !bUsePriority || GetAIHandle()->GetSoundPriority(pCurrent) >= iBestPriority )
				{
					flDist = ( pCurrent->GetSoundOrigin() - earPosition ).LengthSqr();

					if ( flDist < flBestDist )
					{
						pResult = pCurrent;
						flBestDist = flDist;

						iBestPriority = GetAIHandle()->GetSoundPriority(pCurrent);
					}
				}
			}
		}
	}
	
	return pResult;
}

//-----------------------------------------------------------------------------

void CGEAISense::PerformSensing( void )
{
	Look( m_fLookDist );
	Listen();
}


int	CGEAISense::GetEntityCount()
{
	return (m_SeenHighPriority.Count() + m_SeenNPCs.Count() + m_SeenMisc.Count());
}

CBaseEntity* CGEAISense::GetEntity(int index)
{
	if (index <0 || index >= GetEntityCount())
		return NULL;

	if (index > m_SeenHighPriority.Count())
		index -= m_SeenHighPriority.Count();
	else
		return m_SeenHighPriority[index].Get();

	if (index > m_SeenNPCs.Count())
		index -= m_SeenNPCs.Count();
	else
		return m_SeenNPCs[index].Get();

	return m_SeenMisc[index].Get();
}

int	CGEAISense::GetSoundCount()
{
	return m_vSoundList.Count();
}

CSound*	CGEAISense::GetSound(int index)
{
	if (index <0 || index >= m_vSoundList.Count())
		return NULL;

	return CSoundEnt::SoundPointerForIndex( m_vSoundList[index] );
}