///////////// Copyright © 2013 GoldenEye: Source, All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_recipientfilter.h
//   Description :
//      Custom recipient filters for use with Python
//
//   Created On: 05/30/2013
//   Created By: Killermonkey <killermonkey01@gmail.com>
////////////////////////////////////////////////////////////////////////////

#ifndef GE_RECIPIENT_FILTER_H
#define GE_RECIPIENT_FILTER_H

#include "recipientfilter.h"

//-----------------------------------------------------------------------------
// Purpose: Filter based on a team
//-----------------------------------------------------------------------------
class CGETeamRecipientFilter : public CRecipientFilter
{
public:
	CGETeamRecipientFilter( int team, bool isReliable = false );

	int iTeam;
};

#endif