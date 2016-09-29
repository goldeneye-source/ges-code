///////////// Copyright © 2011 GoldenEye: Source. All rights reserved. /////////////
//
//   Project     : Client
//   File        : ge_creategame.cpp
//   Description :
//      Replaces the default create game panel in the menu
//
//   Created On: 11/10/11
//   Created By: Jonathan White <Killermonkey>
////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/CheckButton.h>
#include "ge_panelhelper.h"
#include "filesystem.h"
#include "script_parser.h"
#include "ge_shareddefs.h"
#include "ge_utils.h"

#define RANDOM_VALUE	"_random"
#define COMMAND_MAP		"resource/ges_createserver_map.txt"
#define COMMAND_MAP_VAL	"cfg/ges_serversettings.txt"
#define PYDIR			"python/ges/GamePlay/*.py"

using namespace vgui;

class CGECreateServer : public Frame
{
private:
	DECLARE_CLASS_SIMPLE(CGECreateServer, Frame);

public:
	CGECreateServer( VPANEL parent );
	~CGECreateServer();

	virtual const char *GetName( void ) { return "GESCreateServer"; }

	virtual bool NeedsUpdate() { return false; };
	virtual bool HasInputElements( void ) { return true; }

	virtual void SetVisible( bool state );

	// both Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual void SetParent( Panel* parent ) { BaseClass::SetParent( parent ); }

	void AddWeaponSet( const char *group, KeyValues *set );

protected:
	void PopulateControls();

	virtual void OnCommand( const char *command );

private:
	CUtlDict<CUtlDict<char*, int>*, int> m_WeaponSets;
	KeyValues *m_kvCmdMap;
	KeyValues *m_kvCmdValues;
	bool m_bFirstLoad;
};

static GameUI<CGECreateServer> g_GECreateServer;

// Parser for the weapon loadouts (stripped from ge_loadoutmanger.cpp)
class CGELoadoutParser : public CScriptParser
{
protected:
	void Parse( KeyValues *pKeyValuesData, const char *szFileWithoutEXT )
	{
		const char *group = pKeyValuesData->GetName();
		for ( KeyValues *pKVLoadout = pKeyValuesData->GetFirstSubKey(); pKVLoadout; pKVLoadout = pKVLoadout->GetNextKey() )
		{
			const char *szName = pKVLoadout->GetName();
			if (!Q_strstr(szName, " ") && !Q_strstr(szName, ","))
				g_GECreateServer->AddWeaponSet( group, pKVLoadout );
		}
	};

} GELoadoutParser;


CGECreateServer::CGECreateServer( VPANEL parent ) : BaseClass( NULL, GetName() )
{
	SetParent( parent );

	SetScheme( scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme") );
//	SetProportional( true );

	LoadControlSettings("resource/GESCreateServer.res");

	InvalidateLayout();
	MakePopup();

	SetSizeable( false );
	SetPaintBackgroundEnabled( true );
	SetPaintBorderEnabled( true );
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	
	// Attempt to load the command map (and defaults)
	m_kvCmdMap = new KeyValues( "CommandMap" );
	if ( !m_kvCmdMap->LoadFromFile( filesystem, COMMAND_MAP, "MOD" ) )
		Warning( "Failed to load Create Server map %s\n", COMMAND_MAP );

	// Attempt to load our custom map first
	m_kvCmdValues = new KeyValues( "CommandMapValues" );
	m_kvCmdValues->LoadFromFile( filesystem, COMMAND_MAP_VAL, "MOD" );

	m_bFirstLoad = true;
}

CGECreateServer::~CGECreateServer()
{
	m_WeaponSets.PurgeAndDeleteElements();
	m_kvCmdMap->deleteThis();
	m_kvCmdValues->deleteThis();
}

void CGECreateServer::SetVisible( bool state )
{
	if ( IsVisible() == state )
		return;

	BaseClass::SetVisible( state );

	if ( state )
	{
		PopulateControls();

		CenterThisPanelOnScreen();
		MoveToFront();
		RequestFocus();
		SetEnabled(true);
	}	
}

void CGECreateServer::AddWeaponSet( const char *group, KeyValues *set )
{
	int grpIdx = m_WeaponSets.Find( group );
	if ( grpIdx == m_WeaponSets.InvalidIndex() )
		grpIdx = m_WeaponSets.Insert( group, new CUtlDict<char*, int>() );

	char *name = new char[32];
	Q_strncpy( name, set->GetString("print_name", "Unnamed"), 32 );
	
	int setIdx = m_WeaponSets[grpIdx]->Find( set->GetName() );
	if ( setIdx != m_WeaponSets[grpIdx]->InvalidIndex() )
	{
		// Replace the existing set with this one
		delete [] m_WeaponSets[grpIdx]->Element(setIdx);
		m_WeaponSets[grpIdx]->RemoveAt(grpIdx);
	}

	// Insert the set
	m_WeaponSets[grpIdx]->Insert( set->GetName(), name );
}

void CGECreateServer::OnCommand( const char *command )
{
	if ( !Q_stricmp(command, "play") )
	{
		CUtlVector<char*> commands;

		// Pull the values from our controls and apply them to commands and save off the choices
		for ( KeyValues *kv = m_kvCmdMap->GetFirstTrueSubKey(); kv; kv = kv->GetNextTrueSubKey() )
		{
			KeyValues *kv_value = m_kvCmdValues->FindKey( kv->GetName(), true );
			char *cmd = new char[128];

			try {
				if ( !Q_stricmp(kv->GetString("type"), "CHOICE") )
				{
					ComboBox *panel = dynamic_cast<ComboBox*>( FindChildByName(kv->GetName()) );

					const char *cmd_value = panel->GetActiveItemUserData()->GetName();
					kv_value->SetStringValue( cmd_value );

					if ( !Q_stricmp(cmd_value, RANDOM_VALUE) )
					{
						int idx = GERandom<int>( panel->GetItemCount()-1 ) + 1;
						idx = panel->GetItemIDFromRow( idx );
						cmd_value = panel->GetItemUserData( idx )->GetName();

						// If we picked "random gameplay" be sure we pick random gameplays from then on.
						if (!Q_strcmp(kv->GetString("cmd"), "ge_gameplay"))
							commands.AddToTail("ge_gameplay_mode \"1\""); // Random gameplay selection
					}
					else
					{
						// If we didn't pick "random gameplay" be sure we stick with our original one.
						if (!Q_strcmp(kv->GetString("cmd"), "ge_gameplay"))
							commands.AddToTail("ge_gameplay_mode \"0\""); // Random gameplay selection
					}

					Q_snprintf( cmd, 128, "%s \"%s\"", kv->GetString("cmd"), cmd_value );
					commands.AddToTail( cmd );
				}
				else if ( !Q_stricmp(kv->GetString("type"), "TEXT") )
				{
					char cmd_value[64];
					TextEntry *panel = dynamic_cast<TextEntry*>( FindChildByName(kv->GetName()) );
					panel->GetText( cmd_value, 64 );

					// We don't allow blank values... use default instead
					if ( !cmd_value[0] )
						Q_strncpy( cmd_value, kv->GetString("default",""), 64 );

					kv_value->SetStringValue( cmd_value );

					Q_snprintf( cmd, 128, "%s \"%s\"", kv->GetString("cmd"), cmd_value );
					commands.AddToTail( cmd );
				}
				else if ( !Q_stricmp(kv->GetString("type"), "BOOL") )
				{
					CheckButton *panel = dynamic_cast<CheckButton*>( FindChildByName(kv->GetName()) );
					
					if ( panel->IsSelected() ) {
						kv_value->SetStringValue( "1" );
						Q_snprintf( cmd, 128, "%s \"%s\"", kv->GetString("cmd"), kv->GetString("on_val","1") );
					} else {
						kv_value->SetStringValue( "0" );
						Q_snprintf( cmd, 128, "%s \"%s\"", kv->GetString("cmd"), kv->GetString("off_val","0") );
					}
				
					commands.AddToTail( cmd );
				}
				else
				{
					delete [] cmd;
				}
			} catch (...) {
				delete [] cmd;
			}
		}

		// Apply the commands
		for ( int i=0; i < commands.Count(); i++ )
			engine->ClientCmd_Unrestricted( commands[i] );
		
		// Save our last used settings to our custom file
		m_kvCmdValues->SaveToFile( filesystem, COMMAND_MAP_VAL, "MOD" );

		// Cleanup
		commands.PurgeAndDeleteElements();
	}

	SetVisible( false );
}

void CGECreateServer::PopulateControls( void )
{
	// Only populate on first load
	if ( !m_bFirstLoad )
		return;

	// Populate the map list
	ComboBox *maplist = dynamic_cast<ComboBox*>( FindChildByName("MapList") );
	if ( maplist )
	{
		// Clear the list first
		maplist->DeleteAllItems();
		
		FileFindHandle_t findHandle; // note: FileFINDHandle
		char file[32];

		maplist->AddItem( "#SERVER_RANDOM_MAP", new KeyValues(RANDOM_VALUE) );
		const char *pFilename = filesystem->FindFirstEx( "maps/*.bsp", "MOD", &findHandle );
		while ( pFilename )
		{
			if ( stricmp(pFilename, "ge_transition.bsp") ) //They don't need to pick our dinky crash avoidance map.
			{
				// Add the map to the list
				Q_FileBase(pFilename, file, 32);
				maplist->AddItem(file, new KeyValues(file));
			}

			pFilename = filesystem->FindNext( findHandle );
		}

		filesystem->FindClose( findHandle );
		maplist->SetNumberOfEditLines( 10 );
		maplist->SetEditable( false );
		maplist->GetMenu()->ForceCalculateWidth();
		maplist->ActivateItemByRow( 0 );
	}

	// Populate the weapon list
	ComboBox *weaponlist = dynamic_cast<ComboBox*>( FindChildByName("WeaponList") );
	if ( weaponlist )
	{
		weaponlist->DeleteAllItems();

		// TAKEN DIRECTLY FROM ge_loadoutmanager.cpp
		// Parsing individually allows us to overwrite the default sets with custom ones
		// Multiple custom sets can be defined as needed (can even make sets per gameplay)
		if ( !GELoadoutParser.HasBeenParsed() )
		{
			GELoadoutParser.InitParser("scripts/loadouts/weapon_sets_default.X");
			GELoadoutParser.SetHasBeenParsed( false );
			GELoadoutParser.InitParser("scripts/loadouts/weapon_sets_custom*.X");
		}

		// Random loadout
		weaponlist->AddItem( "#SERVER_RANDOM_SET", new KeyValues("random_loadout") );
	
		// Default sets should appear first.
		int didx = m_WeaponSets.Find("Default Sets");

		if (didx != -1)
		{
			int id = weaponlist->AddItem(m_WeaponSets.GetElementName(didx), NULL);
			weaponlist->GetMenu()->SetItemEnabled(id, false);

			for (int k = m_WeaponSets[didx]->First(); k != m_WeaponSets[didx]->InvalidIndex(); k = m_WeaponSets[didx]->Next(k))
				weaponlist->AddItem(m_WeaponSets[didx]->Element(k), new KeyValues(m_WeaponSets[didx]->GetElementName(k)));
		}
		else
			Warning("Could not find default sets!\n");
	
		FOR_EACH_DICT( m_WeaponSets, idx )
		{
			if (Q_strstr(m_WeaponSets.GetElementName(idx), "_mhide")) // We don't want to see it.
				continue;

			if (!Q_strcmp(m_WeaponSets.GetElementName(idx), "Default Sets")) // Already added this!
				continue;

			int id = weaponlist->AddItem( m_WeaponSets.GetElementName(idx), NULL );
			weaponlist->GetMenu()->SetItemEnabled( id, false );

			for ( int k=m_WeaponSets[idx]->First(); k != m_WeaponSets[idx]->InvalidIndex(); k = m_WeaponSets[idx]->Next(k) )
				weaponlist->AddItem( m_WeaponSets[idx]->Element(k), new KeyValues(m_WeaponSets[idx]->GetElementName(k)) );
		}

		weaponlist->SetEditable( false );
		weaponlist->SetNumberOfEditLines( 15 );
		weaponlist->GetMenu()->ForceCalculateWidth();
		weaponlist->ActivateItemByRow( 0 );
	}

	// Populate the scenario list
	ComboBox *scenariolist = dynamic_cast<ComboBox*>( FindChildByName("ScenarioList") );
	if ( scenariolist )
	{
		// Clear the list first
		scenariolist->DeleteAllItems();

		FileFindHandle_t findHandle; // note: FileFINDHandle
		char file[32];

		scenariolist->AddItem( "#SERVER_RANDOM_SCENARIO", new KeyValues(RANDOM_VALUE) );
		const char *pFilename = filesystem->FindFirstEx( PYDIR, "MOD", &findHandle );
		while ( pFilename )
		{
			// Add the scenario to the list if not __init__
			if ( !Q_stristr(pFilename, "__init__") && !Q_stristr(pFilename, "tournamentdm") )
			{
				Q_FileBase( pFilename, file, 32 );
				scenariolist->AddItem( file, new KeyValues(file) );
			}

			pFilename = filesystem->FindNext( findHandle );
		}

		filesystem->FindClose( findHandle );
		scenariolist->SetEditable( false );
		scenariolist->SetNumberOfEditLines( 10 );
		scenariolist->GetMenu()->ForceCalculateWidth();
		scenariolist->ActivateItemByRow( 0 );
	}

	// Populate the bot difficulty list
	ComboBox *botdifflist = dynamic_cast<ComboBox*>( FindChildByName("BotLevel") );
	if ( botdifflist && botdifflist->GetItemCount() == 0 )
	{
		// Hard coded items (sorry!)
		botdifflist->AddItem( "#BOT_LEVEL_EASY", new KeyValues("1") );
		botdifflist->AddItem( "#BOT_LEVEL_MED", new KeyValues("3") );
		botdifflist->AddItem( "#BOT_LEVEL_HARD", new KeyValues("6") );
		botdifflist->AddItem( "#BOT_LEVEL_UBER", new KeyValues("9") );

		// Admin
		botdifflist->SetEditable( false );
		botdifflist->SetNumberOfEditLines( 4 );
		botdifflist->GetMenu()->ForceCalculateWidth();
		botdifflist->ActivateItemByRow( 0 );
	}

	// Populate the turbo mode list
	ComboBox *turbolist = dynamic_cast<ComboBox*>( FindChildByName("TurboMode") );
	if ( turbolist && turbolist->GetItemCount() == 0 )
	{
		// Hard coded items (sorry!)
		turbolist->AddItem( "#TURBO_MODE_NORM", new KeyValues("1.000000") );
		turbolist->AddItem( "#TURBO_MODE_FAST", new KeyValues("1.500000") );
		turbolist->AddItem( "#TURBO_MODE_LIGHT", new KeyValues("1.850000") );
		
		// Admin
		turbolist->SetEditable( false );
		turbolist->SetNumberOfEditLines( 3 );
		turbolist->GetMenu()->ForceCalculateWidth();
		turbolist->ActivateItemByRow( 0 );
	}

	// Load the default/saved values from our command map
	for ( KeyValues *kv = m_kvCmdMap->GetFirstTrueSubKey(); kv; kv = kv->GetNextTrueSubKey() )
	{
		// Get our value (or default)
		const char *value = (m_kvCmdValues->FindKey( kv->GetName() )) ? m_kvCmdValues->GetString( kv->GetName() ) : NULL;
		if ( !value )
			value = kv->GetString( "default", "" );

		if ( !Q_stricmp(kv->GetString("type"), "CHOICE") )
		{
			ComboBox *panel = dynamic_cast<ComboBox*>( FindChildByName(kv->GetName()) );
			if ( !panel )
				continue;

			// Search through all our items to find a matching value
			for ( int i=0; i < panel->GetItemCount(); i++ )
			{
				int id = panel->GetItemIDFromRow( i );
				KeyValues* userdata = panel->GetItemUserData( id );
				
				if ( userdata && !Q_stricmp(value, userdata->GetName()) )
				{
					panel->ActivateItem( id );
					break;
				}
			}
		}
		else if ( !Q_stricmp(kv->GetString("type"), "TEXT") )
		{
			TextEntry *panel = dynamic_cast<TextEntry*>( FindChildByName(kv->GetName()) );
			if ( !panel )
				continue;

			panel->SetText( value );
		}
		else if ( !Q_stricmp(kv->GetString("type"), "BOOL") )
		{
			CheckButton *panel = dynamic_cast<CheckButton*>( FindChildByName(kv->GetName()) );
			if ( !panel )
				continue;

			if ( !Q_stricmp(value, "1") )
				panel->SetSelected( true );
			else
				panel->SetSelected( false );
		}
	}

	m_bFirstLoad = false;
}

CON_COMMAND( showcreateserver, "Brings up the GE:S Create Server panel" )
{
	g_GECreateServer->SetVisible( !g_GECreateServer->IsVisible() );
}
