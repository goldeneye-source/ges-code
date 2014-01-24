///////////// Copyright © 2013 GoldenEye: Source, All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_recipientfilter.cpp
//   Description :
//      Custom recipient filters for use with Python
//
//   Created On: 05/30/2013
//   Created By: Killermonkey <killermonkey01@gmail.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_shareddefs.h"
#include "ge_recipientfilter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGETeamRecipientFilter::CGETeamRecipientFilter( int team, bool isReliable )
{
	if (isReliable)
		MakeReliable();

	RemoveAllRecipients();

	// Include observers in this filter
	bool inc_observers = true;
	// ONLY observers are allowed
	bool only_observers = false;
	// Ignore teams (for TEAM_OBS and TEAM_NO_OBS)
	bool ignore_team = false;

	// Translate the team
	if ( team > MAX_GE_TEAMS )
	{
		switch ( team )
		{
		case TEAM_OBS:
			// Only observers on unassigned team
			ignore_team = true;
			break;
		case TEAM_NO_OBS:
			// No observers on unassigned team
			inc_observers = false;
			ignore_team = true;
			break;
		case TEAM_MI6_OBS:
			// Only observers on MI6
			only_observers = true;
			team = TEAM_MI6;
			break;
		case TEAM_MI6_NO_OBS:
			// No observers on MI6
			inc_observers = false;
			team = TEAM_MI6;
			break;
		case TEAM_JANUS_OBS:
			// Only observers on Janus
			only_observers = true;
			team = TEAM_JANUS;
			break;
		case TEAM_JANUS_NO_OBS:
			// No observers on Janus
			inc_observers = false;
			team = TEAM_JANUS;
			break;
		}
	}

	this->iTeam = team;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;

		// Check if we are ignoring the team check
		if ( ignore_team )
		{
			if ( inc_observers && !pPlayer->IsObserver() )
				continue;
			else if ( !inc_observers && pPlayer->IsObserver() )
				continue;
		}
		else
		{
			// Exclude observers up front if we don't even want them (reduces logic below)
			if ( pPlayer->IsObserver() && !inc_observers )
				continue;

			if ( pPlayer->GetTeamNumber() != team )
			{
				// If we are not on the desired team, then see if we are a spectator
				if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
				{
					// If only_observers == false, than we need to check if they are watching through the user's eyes
					// this is the default behavior of CTeamRecipientFilter
					if ( !only_observers )
					{
						// This is a normal request (i.e., inc_observers == true, only_observers == false)
						if ( pPlayer->GetObserverTarget() && (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE || pPlayer->GetObserverMode() == OBS_MODE_CHASE) )
						{
							if ( pPlayer->GetObserverTarget()->GetTeamNumber() != team )
								continue;
						}
						else
						{
							// We aren't watching this team
							continue;
						}
					}

					// NOTE: If only_observers == true then we fall through to AddRecipient!
				}
				else
				{
					// Off-team, Non-spectators always get the boot
					continue;
				}
			}

			// Check if we only want observers on this team
			if ( !pPlayer->IsObserver() && only_observers )
				continue;			
		}

		AddRecipient( pPlayer );
	}
}