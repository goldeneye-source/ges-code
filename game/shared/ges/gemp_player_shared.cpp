///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : gemp_player_shared.cpp
//   Description :
//      [TODO: Write the purpose of gemp_player_shared.cpp.]
//
//   Created On: 2/20/2009 3:24:14 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_gemp_player.h"
#include "prediction.h"
#define CRecipientFilter C_RecipientFilter
#else
#include "gemp_player.h"
#include "ilagcompensationmanager.h"
#include "gamestats.h"
#endif

#include "in_buttons.h"
#include "ge_gamerules.h"
#include "ge_weapon.h"
#include "gemp_gamerules.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


bool CGEMPPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{	
	// Rebel owned projectiles (grenades etc) and players will collide.	
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{		
		if ( GetTeamNumber() == TEAM_MI6 &&  contentsMask & CONTENTS_TEAM1 )			
			return false;		
		else if ( GetTeamNumber() == TEAM_JANUS &&  contentsMask & CONTENTS_TEAM2 )			
			return false;	
	}	

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}
