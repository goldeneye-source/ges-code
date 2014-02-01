///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// ge_hudtargetid.cpp
//
// Description:
//      Ripped from valve and tweaked for enemy distance.
//		Display's target's name and health (friendly only)
//
// Created On: 11/9/2008
// Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/Controls.h"

#include "c_ge_player.h"
#include "c_gebot_player.h"
#include "c_ge_playerresource.h"

#include "ge_gamerules.h"
#include "ge_shareddefs.h"
#include "ge_utils.h"
#include "gemp_gamerules.h"

#include "ge_hudtargetid.h"

// Used for team icons
#include "voice_status.h"
#include "view.h"
#include "c_team.h"
#include "c_ge_radarresource.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"

#include "engine/ivdebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FRIENDLY_HINT_DISTANCE	1800  // 75 ft
#define ENEMY_HINT_DISTANCE		600  // 50 ft

static ConVar hud_centerid( "hud_centerid", "1" );
static ConVar hud_showtargetid( "hud_showtargetid", "1" );

DECLARE_HUDELEMENT( CGETargetID );

#define MAX_ID_STRING 256

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGETargetID::CGETargetID( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "GETargetID" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;

	m_pMI6Icon = m_pJanusIcon = NULL;

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

CGETargetID::~CGETargetID()
{
	if ( m_pJanusIcon )
		m_pJanusIcon->DecrementReferenceCount();
	if ( m_pMI6Icon )
		m_pMI6Icon->DecrementReferenceCount();
}

void CGETargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_hFont = scheme->GetFont( "GETargetID", IsProportional() );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CGETargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;

	// Load up our icons
	CHudTexture *icon = gHUD.GetIcon( "ic_overhead_janus" );
	if ( icon && !m_pJanusIcon )
	{
		m_pJanusIcon = materials->FindMaterial( icon->szTextureFile, TEXTURE_GROUP_VGUI );
		m_pJanusIcon->IncrementReferenceCount();
	}

	icon = gHUD.GetIcon( "ic_overhead_mi6" );
	if ( icon && !m_pMI6Icon )
	{
		m_pMI6Icon = materials->FindMaterial( icon->szTextureFile, TEXTURE_GROUP_VGUI );
		m_pMI6Icon->IncrementReferenceCount();
	}
}

Color CGETargetID::GetColorForTargetTeam( int iTeamNumber )
{
	return GameResources()->GetTeamColor( iTeamNumber );
} 

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CGETargetID::Paint()
{
	C_GEPlayer *pPlayer = (C_GEPlayer*)C_GEPlayer::GetLocalPlayer();
	if ( !pPlayer || !GEMPRules()->IsTeamplay() )
		return;

	// Get our target's ent index
	int iEntIndex = pPlayer->GetIDTarget();

	// Didn't find one?
	if ( !iEntIndex )
	{
		// Check to see if we should clear our ID
		if ( m_flLastChangeTime && (gpGlobals->curtime > (m_flLastChangeTime + 0.5)) )
		{
			m_flLastChangeTime = 0;
			m_iLastEntIndex = 0;
		}
		else
		{
			// Keep re-using the old one
			iEntIndex = m_iLastEntIndex;
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;
	}

	// The ToGEPlayer function should automatically handle bots in disguise!
	C_GEPlayer *pTarget = ToGEPlayer( cl_entitylist->GetEnt(iEntIndex) );
	if ( pTarget )
	{
		C_GEPlayer *pLocalPlayer = ToGEPlayer( C_BasePlayer::GetLocalPlayer() );
		
		if ( pTarget->InSameTeam(pLocalPlayer) && pTarget->GetAbsOrigin().DistTo( pLocalPlayer->GetAbsOrigin() ) <= FRIENDLY_HINT_DISTANCE )
		{
			// Proceed with putting their name up if we are in distance
			Color c = GetColorForTargetTeam( pTarget->GetTeamNumber() );
			wchar_t sIDString[MAX_ID_STRING];
			sIDString[0] = 0;

			const char *printFormatString = "#Playerid_sameteam";
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			wchar_t wszHealthText[10], wszArmorText[10];

			char name[64];
			Q_strncpy( name, pTarget->GetPlayerName(), 64 );

			g_pVGuiLocalize->ConvertANSIToUnicode( GEUTIL_RemoveColorHints(name), wszPlayerName, sizeof(wszPlayerName) );

			float amt;
			if ( pTarget->GetMaxHealth() == 0 )
				amt = 0;
			else
				amt = ((float)pTarget->GetHealth() / (float)pTarget->GetMaxHealth() ) * 100.0f;

			_snwprintf( wszHealthText, ARRAYSIZE(wszHealthText) - 1, L"%0.0f%%", amt );
			wszHealthText[ ARRAYSIZE(wszHealthText)-1 ] = '\0';

			if ( pTarget->GetMaxArmor() == 0 )
				amt = 0;
			else
				amt = ((float)pTarget->GetArmor() / (float)pTarget->GetMaxArmor() ) * 100.0f;

			_snwprintf( wszArmorText, ARRAYSIZE(wszArmorText) - 1, L"%0.0f%%", amt );
			wszArmorText[ ARRAYSIZE(wszArmorText)-1 ] = '\0';
		
			g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString), g_pVGuiLocalize->Find(printFormatString), 3, wszPlayerName, wszHealthText, wszArmorText );

			int wide, tall;
			int ypos = YRES(260);
			int xpos = XRES(10);

			vgui::surface()->GetTextSize( m_hFont, sIDString, wide, tall );

			if( hud_centerid.GetInt() == 0 )
			{
				ypos = YRES(400);
			}
			else
			{
				xpos = (ScreenWidth() - wide) / 2;
			}
			
			vgui::surface()->DrawSetTextFont( m_hFont );
			vgui::surface()->DrawSetTextPos( xpos, ypos );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawPrintText( sIDString, wcslen(sIDString) );
		}
	}
}

extern bool GE_IsLocalObjective( C_BaseEntity *pEnt );
extern float g_flHeadIconSize;
extern float g_flHeadOffset;
#define TEAM_ICON_SIZE		20		// Diameter
#define TEAM_ICON_ALPHA		75		// 0-255
#define TEAM_ICON_MAXDIST	300		// Feet
#define TEAM_ICON_MINDIST	40		// Feet
#define TEAM_ICON_OFFSET	20		// Inches

void CGETargetID::DrawOverheadIcons()
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	// We don't draw these for none teamplay
	if ( pLocalPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		return;

	Vector vLocalOrigin = pLocalPlayer->EyePosition();
	float maxdist = TEAM_ICON_MAXDIST * 12.0f;
	float mindist = TEAM_ICON_MINDIST * 12.0f;

	// Go through and draw overhead icons as needed
	C_Team *myTeam = pLocalPlayer->GetTeam();
	for ( int i=0; i < myTeam->GetNumPlayers(); i++ )
	{
		unsigned int alpha = clamp( TEAM_ICON_ALPHA, 0, 255 );

		// Get our teammate instance
		C_BaseCombatCharacter *pPlayer = myTeam->GetPlayer( i );

		// Don't show an icon if the player has never entered our PVS
		if ( !pPlayer || pPlayer == pLocalPlayer || !pPlayer->IsAlive() )
			continue;

		// Don't draw if we are speaking
		if ( GetClientVoiceMgr()->IsPlayerSpeaking( pPlayer->entindex() ) )
			continue;

		// Don't conflict with objective icons
		if ( GE_IsLocalObjective( pPlayer ) )
			continue;

		// Grab our real player handle in case we are a bot
		const CBaseHandle hPlayer = pPlayer->GetRefEHandle();

		// Divert to the NPC if we are a bot for proper drawing position
		if ( pPlayer->GetFlags() & FL_FAKECLIENT )
		{
			C_GEBotPlayer *pBot = dynamic_cast<C_GEBotPlayer*>( pPlayer );
			if ( pBot )
				pPlayer = pBot->GetNPC();
		}

		float flOffset = TEAM_ICON_OFFSET;
		Vector vPlayerOrigin;
		// If we are outside of PVS use our radar data as the origin :-D
		// NOTE: No need for player specific offsets
		if ( pPlayer->IsDormant() && g_RR )
		{
			int idx = g_RR->FindEntityIndex( hPlayer.ToInt() );
			vPlayerOrigin = g_RR->GetOrigin( idx );
			vPlayerOrigin.z += pPlayer->WorldAlignSize().z + flOffset - 9.65f;
		}
		else
		{
			pPlayer->GetAttachment( "anim_attachment_head", vPlayerOrigin );
			vPlayerOrigin.z += flOffset;
		}

		float dist = vPlayerOrigin.DistTo(vLocalOrigin);
 		if ( maxdist > 0 && dist > maxdist )
			continue;
		else if ( dist > 1200 )
			alpha = (unsigned int) (alpha * 1.25);

		float flSize = TEAM_ICON_SIZE * 0.5f;

		// Check for line of sight (if > minimum distance)
		trace_t tr;
		UTIL_TraceLine( vLocalOrigin, vPlayerOrigin + Vector(0,0,flSize), MASK_BLOCKLOS, pLocalPlayer, COLLISION_GROUP_NONE, &tr );
		
		// If we are blocked check if we are in min distance and decrease alpha otherwise don't draw
		if ( tr.fraction != 1.0 )
		{
			if ( dist < mindist )
				alpha /= 2;
			else
				continue;
		}

		// The middle of the image starts with our origin from above
		Vector vOrigin = vPlayerOrigin;
		vPlayerOrigin.z += flSize;

		// Align it so it never points up or down.
		Vector vUp( 0, 0, 1 );
		Vector vRight = CurrentViewRight();
		if ( fabs( vRight.z ) > 0.95 )	// don't draw it edge-on
			continue;

		vRight.z = 0;
		VectorNormalize( vRight );

		CMatRenderContextPtr pRenderContext( materials );

		if ( pPlayer->GetTeamNumber() == TEAM_MI6 )
			pRenderContext->Bind( m_pMI6Icon );
		else
			pRenderContext->Bind( m_pJanusIcon );

		IMesh *pMesh = pRenderContext->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Color4ub( 255, 255, 255, alpha );
		meshBuilder.TexCoord2f( 0,0,0 );
		meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * flSize)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( 255, 255, 255, alpha );
		meshBuilder.TexCoord2f( 0,1,0 );
		meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * flSize)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( 255, 255, 255, alpha );
		meshBuilder.TexCoord2f( 0,1,1 );
		meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * -flSize)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( 255, 255, 255, alpha );
		meshBuilder.TexCoord2f( 0,0,1 );
		meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * -flSize)).Base() );
		meshBuilder.AdvanceVertex();
		meshBuilder.End();
		pMesh->Draw();
	}
}