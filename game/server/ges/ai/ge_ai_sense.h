///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai_sense.h
//   Description :
//      [TODO: Write the purpose of ge_ai_sense.h.]
//
//   Created On: 3/8/2009 12:01:17 PM
//   Created By:  <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GE_AI_SENSE_H
#define MC_GE_AI_SENSE_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_ai_helper.h"
#include "soundent.h"

class CGEAISense : public CGEAIHelper
{
public:
	CGEAISense(CGEAIHandle* baseAi);
	~CGEAISense();

	float			GetDistLook() const				{ return m_fLookDist; }
	void			SetDistLook( float flDistLook ) { m_fLookDist = flDistLook; }

	void			PerformSensing();

	void			Listen( void );
	void			Look( int iDistance );// basic sight function for npcs

	bool			ShouldSeeEntity( CBaseEntity *pEntity ); // logical query
	bool			CanSeeEntity( CBaseEntity *pSightEnt ); // more expensive cone & raycast test
	bool			DidSeeEntity( CBaseEntity *pSightEnt ); //  a less expensive query that looks at cached results from recent conditionsa gathering

	//CBaseEntity *	GetFirstSeenEntity( AISightIter_t *pIter, seentype_t iSeenType = SEEN_ALL ) const;
	//CBaseEntity *	GetNextSeenEntity( AISightIter_t *pIter ) const;

	//CSound *		GetFirstHeardSound( AISoundIter_t *pIter );
	//CSound *		GetNextHeardSound( AISoundIter_t *pIter );

	int				GetEntityCount();
	CBaseEntity*	GetEntity(int index);

	int				GetSoundCount();
	CSound*			GetSound(int index);


	CSound *		GetClosestSound( bool fScent = false, int validTypes = ALL_SOUNDS | ALL_SCENTS, bool bUsePriority = true );

	bool 			CanHearSound( CSound *pSound );

	
	float			GetTimeLastUpdate( CBaseEntity *pEntity );

protected:
	void			BeginGather();
	void 			NoteSeenEntity( CBaseEntity *pSightEnt );
	void			EndGather( int nSeen, CUtlVector<EHANDLE> *pResult );
	
	bool 			Look( CBaseEntity *pSightEnt );
	int 			LookForHighPriorityEntities( int iDistance );
	int 			LookForNPCs( int iDistance );
	int 			LookForObjects( int iDistance );
	
	bool			SeeEntity( CBaseEntity *pEntity );


private:
	CUtlVector<int>	m_vSoundList;

	CUtlVector<EHANDLE> m_SeenHighPriority;
	CUtlVector<EHANDLE> m_SeenNPCs;
	CUtlVector<EHANDLE> m_SeenMisc;
	CUtlVector<EHANDLE> *m_SeenArrays[3];
	
	float			m_fLookDist;				// distance npc sees (Default 2048)
	float			m_fLastLookDist;
	float			m_fTimeLastLook;

	float			m_fTimeLastLookHighPriority;
	float			m_fTimeLastLookNPCs;
	float			m_fTimeLastLookMisc;
};


#endif //MC_GE_AI_SENSE_H
