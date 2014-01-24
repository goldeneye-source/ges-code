#include "cbase.h"
#include "Sprite.h"
#include "utlbuffer.h"
#include "filesystem.h"
#include "ai_network.h"
#include "ai_networkmanager.h"

static bool bInNodePlacement = false;
static CUtlVector<CBaseEntity*> pNodes;

static int NODE_MASK = MASK_NPCSOLID_BRUSHONLY | CONTENTS_PLAYERCLIP;

CBaseEntity *CreateNode( Vector origin )
{
	CSprite *pEnt = CSprite::SpriteCreate( "sprites/glow01.vmt", origin, false );
	pEnt->SetScale( 0.35f );

	pNodes.AddToTail(pEnt);
	return pEnt;
}

void RebuildNodeGraph()
{
	UTIL_RemoveImmediate( g_pAINetworkManager );
	CAI_NetworkManager::DeleteAllAINetworks();
	CAI_NetworkManager::InitializeAINetworks();
}

ConVar ge_ai_nodesep( "ge_node_linesep", "120", FCVAR_GAMEDLL, "Nominal distance between nodes when using ge_node_addline" );

CON_COMMAND( ge_node_start, "Start placement of nodes for custom nodegraph" )
{
	if ( bInNodePlacement )
		return;

	// if the text file already exists, load its current nodes
	char szNodeTextFilename[MAX_PATH];
	Q_snprintf( szNodeTextFilename, sizeof( szNodeTextFilename ), "maps/graphs/%s%s.txt", STRING( gpGlobals->mapname ), GetPlatformExt() );
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

	if ( filesystem->ReadFile( szNodeTextFilename, "game", buf ) )
	{
		const int maxLen = 64;
		char line[ maxLen ];
		CUtlVector<char*> floats;
		int num = 0;

		// loop through every line of the file, read it in
		while( true )
		{
			buf.GetLine( line, maxLen );
			if ( Q_strlen(line) <= 0 )
				break; // reached the end of the file

			// we've read in a string containing 3 tab separated floats
			// we need to split this into 3 floats, which we put in a vector
			V_SplitString( line, "	", floats );
			Vector origin( atof( floats[0] ), atof( floats[1] ), atof( floats[2] ) );

			floats.PurgeAndDeleteElements();

			CreateNode( origin );
			num++;

			if ( !buf.IsValid() )
				break;
		}
	}

	bInNodePlacement = true;
	UTIL_ClientPrintAll( HUD_PRINTTALK, "Entered node placement mode\n" );
}

CON_COMMAND( ge_node_save, "Save placed nodes, ends placement" )
{
	if ( !bInNodePlacement )
		return;

	// save the nodes
	char szNodeTextFilename[MAX_PATH];
	Q_snprintf( szNodeTextFilename, sizeof( szNodeTextFilename ),
				"maps/graphs/%s%s.txt", STRING( gpGlobals->mapname ), GetPlatformExt() );

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	for ( int i=0; i<pNodes.Size(); i++ )
	{
		buf.PutString( UTIL_VarArgs("%f	%f	%f\n",pNodes[i]->GetAbsOrigin().x,
							 pNodes[i]->GetAbsOrigin().y,
							 pNodes[i]->GetAbsOrigin().z) );
	}
	filesystem->WriteFile(szNodeTextFilename,"game",buf);

	// clean up & exit node mode
	for ( int i=0; i < pNodes.Count(); i++ )
		UTIL_RemoveImmediate( pNodes[i] );

	pNodes.RemoveAll();
	bInNodePlacement = false;
	UTIL_ClientPrintAll( HUD_PRINTTALK, "Saved nodes & exited node placement mode, rebuilding graph!\n" );

	RebuildNodeGraph();
}

CON_COMMAND( ge_node_addlook, "Add a node where you are looking" )
{
	if ( !bInNodePlacement )
	{
		Warning( "You are not in node placement mode! Use ge_node_start to enter it!\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( UTIL_GetCommandClientIndex() );
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );

	UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, NODE_MASK, 
			pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0f && abs(tr.plane.normal.z) > 0.6f )
	{
		CreateNode( tr.endpos + Vector(0,0,10) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, "Failed to create node\n" );
	}
}

CON_COMMAND( ge_node_add, "Add a node where you are standing" )
{
	if ( !bInNodePlacement )
	{
		Warning( "You are not in node placement mode! Use ge_node_start to enter it!\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( UTIL_GetCommandClientIndex() );

	if ( pPlayer->GetGroundEntity() )
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + Vector(0,0,10);
		CreateNode( vecOrigin );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, "Failed to create node\n" );
	}
}

CON_COMMAND( ge_node_remove, "Delete a node you are looking at" )
{
	if ( !bInNodePlacement )
	{
		Warning( "You are not in node placement mode! Use ge_node_start to enter it!\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( UTIL_GetCommandClientIndex() );
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );

	UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, NODE_MASK, 
			pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0 )
	{
		CBaseEntity *list[256];
		Vector mins = tr.endpos - Vector( 32, 32, 4 );
		Vector maxs = tr.endpos + Vector( 32, 32, 32 );

		int count = UTIL_EntitiesInBox( list, 256, mins, maxs, 0 );
		for ( int i=0; i < count; i++ )
		{
			if ( list[i] && !Q_stricmp( list[i]->GetClassname(), "env_sprite" ) )
			{
				pNodes.FindAndRemove( list[i] );
				UTIL_RemoveImmediate( list[i] );
			}
		}
	}
}

CON_COMMAND( ge_node_addline, "Adds nodes in a line from you to where you are looking spaced apart equally" )
{
	if ( !bInNodePlacement )
	{
		Warning( "You are not in node placement mode! Use ge_node_start to enter it!\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( UTIL_GetCommandClientIndex() );
	trace_t tr;
	Vector offset(0,0,10);
	Vector forward;
	pPlayer->EyeVectors( &forward );

	UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, NODE_MASK, 
			pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0 )
	{
		Vector start = pPlayer->EyePosition();
		Vector end   = tr.endpos - forward*32.0f;
		float dist   = start.DistTo( end );

		// Create the start node
		UTIL_TraceLine( start, start - Vector(0,0,1) * MAX_TRACE_LENGTH, NODE_MASK, pPlayer, COLLISION_GROUP_NONE, &tr );
		CreateNode( tr.endpos + offset );

		// Take care of small distances by just placing one node under us
		if ( dist <= ge_ai_nodesep.GetFloat() / 2.0f )
			return;

		// Place nodes spaced roughly 120 units apart from start to end
		int nodes = dist / ge_ai_nodesep.GetFloat();
		float step = dist / (nodes + 1);

		Vector pos;
		for ( int i=1; i <= nodes; i++ )
		{
			// Find our position along the ray and shoot it to the floor
			pos = start + forward * step * i;
			UTIL_TraceLine( pos, pos - Vector(0,0,1) * MAX_TRACE_LENGTH, NODE_MASK, pPlayer, COLLISION_GROUP_NONE, &tr );

			// Create the node 10 units above the floor
			CreateNode( tr.endpos + offset );
		}

		// Create the end node
		UTIL_TraceLine( end, end - Vector(0,0,1) * MAX_TRACE_LENGTH, NODE_MASK, pPlayer, COLLISION_GROUP_NONE, &tr );
		CreateNode( tr.endpos + offset );
	}
}

CON_COMMAND( ge_node_undo, "Undo the last node placed" )
{
	if ( !bInNodePlacement )
	{
		Warning( "You are not in node placement mode! Use ge_node_start to enter it!\n" );
		return;
	}

	if ( pNodes.Size() > 0 )
	{
		UTIL_Remove( pNodes.Tail() );
		pNodes.Remove( pNodes.Size()-1 );

		UTIL_ClientPrintAll( HUD_PRINTTALK, "Last node removed\n" );
	}
}

CON_COMMAND( ge_node_rebuildgraph, "Rebuilds the node graph" )
{
	RebuildNodeGraph();
}