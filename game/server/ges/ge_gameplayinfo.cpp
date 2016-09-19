//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// ge_gameplayinfo.cpp
//
// Description:
//     See Header
//
// Created On: 6/11/2011
// Created By: Killer Monkey
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "gemp_gamerules.h"
#include "ge_gameplay.h"

class CGEGameplayInfo : public CPointEntity, public CGEGameplayEventListener
{
public:
	DECLARE_CLASS( CGEGameplayInfo, CPointEntity );
	DECLARE_DATADESC();

	CGEGameplayInfo();

	virtual void OnGameplayEvent( GPEvent event );

	void InputGetConnectionCount(inputdata_t &inputdata);
	void InputGetPlayerCount( inputdata_t &inputdata );
	void InputGetRoundCount( inputdata_t &inputdata );

	void Spawn();

	float m_fFloorHeight = 125.0f; //Floor height used for radar and spawning purposes.

	// Gamemode that is checked for on scenario change, firing an output on match/no match.
	string_t m_sGamemode;

	COutputInt m_outConnectionCount;
	COutputInt m_outPlayerCount;
	COutputInt m_outRoundCount;
	COutputEvent m_outTeamplayOn;
	COutputEvent m_outTeamplayOff;
	COutputEvent m_outTeamSpawnsOn;
	COutputEvent m_outTeamSpawnsOff;
	COutputEvent m_outRoundStart;
	COutputEvent m_outRoundEnd;

	//Gamemode check outputs.
	COutputEvent m_OnGameplayTrue;
	COutputEvent m_OnGameplayFalse;
	COutputEvent m_OnSuperfluousAreasEnabled;
	COutputEvent m_OnSuperfluousAreasDisabled;
};

LINK_ENTITY_TO_CLASS( ge_gameplayinfo, CGEGameplayInfo );

BEGIN_DATADESC( CGEGameplayInfo )
	// Parameters
	DEFINE_KEYFIELD( m_fFloorHeight, FIELD_FLOAT, "FloorHeight" ),
	DEFINE_KEYFIELD( m_sGamemode, FIELD_STRING, "SpecGamemode" ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "GetConnectionCount", InputGetConnectionCount),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetPlayerCount", InputGetPlayerCount ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetRoundCount", InputGetRoundCount ),
	
	// Outputs
	DEFINE_OUTPUT( m_outConnectionCount, "ConnectionCount"),
	DEFINE_OUTPUT( m_outPlayerCount, "PlayerCount"),
	DEFINE_OUTPUT( m_outRoundCount,  "RoundCount" ),
	DEFINE_OUTPUT( m_outTeamplayOn,	 "TeamplayOn" ),
	DEFINE_OUTPUT( m_outTeamplayOff, "TeamplayOff" ),
	DEFINE_OUTPUT( m_outTeamSpawnsOn, "TeamSpawnsOn" ),
	DEFINE_OUTPUT( m_outTeamSpawnsOff, "TeamSpawnsOff" ),
	DEFINE_OUTPUT( m_outRoundStart, "RoundStart"),
	DEFINE_OUTPUT( m_outRoundEnd, "RoundEnd"),
	DEFINE_OUTPUT( m_OnGameplayTrue, "OnGameplayMatch"),
	DEFINE_OUTPUT( m_OnGameplayFalse, "OnNoGameplayMatch"),
	DEFINE_OUTPUT( m_OnSuperfluousAreasEnabled, "OnSuperfluousAreasEnabled"),
	DEFINE_OUTPUT( m_OnSuperfluousAreasDisabled, "OnSuperfluousAreasDisabled"),
END_DATADESC()

CGEGameplayInfo::CGEGameplayInfo()
{
	m_fFloorHeight = 125.0f;
}

void CGEGameplayInfo::Spawn(void)
{
	BaseClass::Spawn();

	GEMPRules()->SetMapFloorHeight(m_fFloorHeight);

	DevMsg("Set Map Floorheight to %f \n", m_fFloorHeight);
}

// Called BEFORE players spawn
void CGEGameplayInfo::OnGameplayEvent( GPEvent event )
{
	if ( event == ROUND_START )
	{
		// Call teamplay output
		if ( GEMPRules()->IsTeamplay() )
			m_outTeamplayOn.FireOutput( this, this );
		else
			m_outTeamplayOff.FireOutput( this, this );

		// Fire teamspawns output
		// teamspawns aren't always used in teamplay, mostly for modes like YOLT or CTK.  In these cases spawn areas
		// might need to be protected where in other team modes they're just indistinct parts of the map where anyone can spawn.
		// That's why the distinction is neccecery.
		if ( GEMPRules()->IsTeamSpawn() && GEMPRules()->IsTeamplay() )
			m_outTeamSpawnsOn.FireOutput(this, this);
		else
			m_outTeamSpawnsOff.FireOutput(this, this);

		// Call players output
		// m_outPlayerCount.Set( GEMPRules()->GetNumActivePlayers(), this, this );

		// Call num rounds output
		// m_outRoundCount.Set( GEGameplay()->GetRoundCount(), this, this );

		GEMPRules()->SetMapFloorHeight(m_fFloorHeight);

		DevMsg("Set Map Floorheight to %f \n", m_fFloorHeight);

		// Check to see if we should seal areas off
		if (GEMPRules()->SuperfluousAreasEnabled())
			m_OnSuperfluousAreasEnabled.FireOutput(this, this);
		else
			m_OnSuperfluousAreasDisabled.FireOutput(this, this);

		// We have to check the gamemode here instead of scenario init because that event happens before the world reload
		// and thus any outputs we fire then would be instantly undone.
		if (m_sGamemode != NULL_STRING)
		{
			CGEBaseScenario *pScenario = GEGameplay()->GetScenario();
			if (!pScenario)
				return;

			if (!Q_strcmp(pScenario->GetIdent(), m_sGamemode.ToCStr()))
				m_OnGameplayTrue.FireOutput(this, this);
			else
				m_OnGameplayFalse.FireOutput(this, this);
		}

		// Call round start output
		m_outRoundStart.FireOutput( this, this );
	}
	else if ( event == ROUND_END )
	{
		m_outRoundEnd.FireOutput( this, this );
	}
}

extern ConVar ge_bot_threshold;

void CGEGameplayInfo::InputGetConnectionCount(inputdata_t &inputdata)
{
	int iNumConnections = 0;

	// Find out how many people are actually connected to the server.
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if (engine->GetPlayerNetInfo(i))
			iNumConnections++;
	}

	// Bots don't have connections but we should still consider them.
	iNumConnections = MAX(MAX(iNumConnections, ge_bot_threshold.GetInt()), GEMPRules()->GetNumAlivePlayers());

	m_outConnectionCount.Set(iNumConnections, inputdata.pActivator, this);
}

void CGEGameplayInfo::InputGetPlayerCount( inputdata_t &inputdata )
{
	m_outPlayerCount.Set( GEMPRules()->GetNumActivePlayers(), inputdata.pActivator, this );
}

void CGEGameplayInfo::InputGetRoundCount( inputdata_t &inputdata )
{
	m_outRoundCount.Set( GEGameplay()->GetRoundCount(), inputdata.pActivator, this );
}
