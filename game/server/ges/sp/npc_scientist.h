///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : npc_scientist.h
//   Description :
//      [TODO: Write the purpose of npc_scientist.h.]
//
//   Created On: 12/27/2008 1:53:29 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_NPC_SCIENTIST_H
#define MC_NPC_SCIENTIST_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_gebase.h"

class CNPC_GESScientist : public CNPC_GEBase
{
public:
	CNPC_GESScientist();

	DECLARE_CLASS( CNPC_GESScientist, CNPC_GEBase );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	Class_T Classify( void );
	const char* GetNPCModel();

	int	SelectSchedule();
	void OnLooked( int iDistance );

	void Spawn( void );
};


#endif //MC_NPC_SCIENTIST_H
