///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai.cpp
//   Description :
//      [TODO: Write the purpose of ge_ai.cpp.]
//
//   Created On: 9/8/2009 8:38:16 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ai_npcstate.h"
#include "ge_ai.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGEAiManager* g_pAiManager = NULL;
CGEAiManager* const GEAi() { return g_pAiManager; }

