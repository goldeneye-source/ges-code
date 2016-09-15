///////////// Copyright © 2010 LodleNet. All rights reserved. /////////////
//
//   Project     : Client
//   File        : ge_updatechecker.cpp
//   Description :
//      [TODO: Write the purpose of ge_updatechecker.cpp.]
//
//   Created On: 3/6/2010 4:44:17 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_updatechecker.h"
#include "ge_shareddefs.h"
#include "ge_utils.h"
#include <vgui/ISurface.h>
#include "vgui_controls/Controls.h"
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/URLLabel.h>
#include <vgui/IVGui.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static GameUI<CGEUpdateChecker> g_GEUpdateChecker;

CGEUpdateChecker::CGEUpdateChecker( vgui::VPANEL parent ) : BaseClass( NULL, GetName() )
{
	m_pWebRequest = NULL;

	SetParent( parent );

	SetScheme( vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme") );
	SetProportional( true );

	LoadControlSettings("resource/GESUpdateTool.res");

	InvalidateLayout();
	MakePopup();

	SetSizeable( false );
	SetPaintBackgroundEnabled( true );
	SetPaintBorderEnabled( true );
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	// Add tick signal to allow for background movies to play
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

CGEUpdateChecker::~CGEUpdateChecker()
{
	if ( m_pWebRequest )
		m_pWebRequest->Destroy();
}

void CGEUpdateChecker::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBgColor( pScheme->GetColor("UpdateBGColor", Color(123,146,156,255)) );

	vgui::PanelListPanel *pLinkList = dynamic_cast<vgui::PanelListPanel*>(FindChildByName("linklist"));
	if ( pLinkList )
	{
		pLinkList->SetFirstColumnWidth(0);
		pLinkList->SetVerticalBufferPixels( YRES(4) );
	}

	vgui::Panel *pPanel = FindChildByName("changelisturl");
	pPanel->SetBorder( pScheme->GetBorder("RaisedBorder") );
}

void CGEUpdateChecker::PostInit()
{
	m_pWebRequest = new CGEWebRequest( GES_VERSION_URL );
}

void CGEUpdateChecker::OnTick()
{
	if ( m_pWebRequest && m_pWebRequest->IsFinished() )
	{
		if ( m_pWebRequest->HadError() )
		{
			Warning( "Failed update check: %s\n", m_pWebRequest->GetError() );
		}
		else
		{
			processUpdate( m_pWebRequest->GetResult() );
		}

		delete m_pWebRequest;
		m_pWebRequest = NULL;

		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
}

void CGEUpdateChecker::processUpdate( const char* data )
{
	KeyValues *kv = new KeyValues("update");
	if ( !kv->LoadFromBuffer( "update", data ) )
	{
		Warning( "Failed update check: Corrupt KeyValues Data\n" );
		return;
	}

	string_t ges_version_text;
	int ges_major_version, ges_minor_version, ges_client_version, ges_server_version;
	if ( !GEUTIL_GetVersion( ges_version_text, ges_major_version, ges_minor_version, ges_client_version, ges_server_version ) )
	{
		Warning( "Failed loading version.txt, your installation may be corrupt!\n" );
		return;
	}
	
	CUtlVector<char*> version;
	Q_SplitString( kv->GetString("version"), ".", version );

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

	// We only need to update if we are behind in the major version, behind in a minor version but equal to the major version, or our client version is behind.
	if (version.Count() >= 3 && (atoi(version[0]) > ges_major_version || (atoi(version[0]) == ges_major_version && atoi(version[1]) > ges_minor_version) || atoi(version[2]) > ges_client_version))
	{
		// We have differing version strings, notify the client
		// Setup the version labels
		vgui::Panel *pVersion = FindChildByName( "yourversion" );
		if ( pVersion )
			PostMessage( pVersion, new KeyValues( "SetText", "text", ges_version_text ) );
	
		pVersion = FindChildByName( "currversion" );
		if ( pVersion )
			PostMessage( pVersion, new KeyValues( "SetText", "text", kv->GetString("version") ) );

		vgui::URLLabel *pChangeList = dynamic_cast<vgui::URLLabel*>( FindChildByName( "changelisturl" ) );
		if ( pChangeList )
			pChangeList->SetURL( kv->GetString( "changelist", "" ) );

		// Setup the list of links
		vgui::PanelListPanel *pLinkList = dynamic_cast<vgui::PanelListPanel*>( FindChildByName( "linklist" ) );
		vgui::URLLabel *pURL;
		if ( pLinkList )
		{
			pLinkList->DeleteAllItems();
			KeyValues *mirrors = kv->FindKey( "client_mirrors" );
			if ( mirrors )
			{
				KeyValues *url = mirrors->GetFirstSubKey();
				while( url )
				{
					pURL = new vgui::URLLabel( NULL, "", url->GetString(), url->GetString() );
					pLinkList->AddItem( NULL, pURL );

					url = url->GetNextKey();
				}
			}
		}

		// Show the update panel
		SetVisible( true );
		SetZPos( 5 );
		RequestFocus();
		MoveToFront();

		// Make sure our background is correct
		SetBgColor( pScheme->GetColor( "UpdateBGColor", Color(123,146,156,255) ) );
	}
}

#ifdef _DEBUG
CON_COMMAND( ge_show_updatechecker, "Show the update checker" )
{
	g_GEUpdateChecker.GetNativePanel()->SetVisible( true );
}
#endif
