///////////// Copyright � 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_musicmanager.cpp
// Description:
//      See Header
//
// Created On: 18 Sep 11
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

// To grab our window handle
#ifdef _WIN32
#pragma warning(disable:4005)
#include <windows.h>
#endif

#include "filesystem.h"
#include "ge_musicmanager.h"
#include "ge_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GE_MUSIC_DEFAULTSCAPE	"__default__"
#define FMOD_TRANSITION_TIME	1.5f
#define CurrTime()  Plat_FloatTime()

#ifdef _WIN32
using namespace FMOD;
#define FMOD_CALL(call) { FMOD_RESULT r = call; if ( r != FMOD_OK ) { DevMsg( "[FMOD] Error Code %i\n", r ); } }
#endif

// -----------------------
// Extern definitions
// -----------------------
CGEMusicManager *g_pGEMusicManager = NULL;
CGEMusicManager *GEMusicManager() { return g_pGEMusicManager; }

HWND gWindowHandle = 0;

void StartGEMusicManager()
{
	if ( g_pGEMusicManager )
		StopGEMusicManager();

#ifdef _WIN32
	// Find our window handle
	gWindowHandle = FindWindow("Valve001", NULL);
#endif

	g_pGEMusicManager = new CGEMusicManager();
	
#ifdef _WIN32
	g_pGEMusicManager->Start();
#endif
}

void StopGEMusicManager()
{
	if (g_pGEMusicManager != NULL)
	{
#ifdef _WIN32
	    if (g_pGEMusicManager->IsAlive())
	    {
	        g_pGEMusicManager->StopThread();
		    g_pGEMusicManager->Join();
	    }
#endif
		delete g_pGEMusicManager;
	}
}

// -----------------------
// Various callbacks
// -----------------------
void GEMusicVolume_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	ConVar *cVar = static_cast<ConVar*>(var);
	GEMusicManager()->SetVolume( cVar->GetFloat() );
}

// -----------------------
// CGEMusicManager
// -----------------------

CGEMusicManager::CGEMusicManager() : CAutoGameSystem( "GEMusicManager" )
{
	m_bRun = false;
	m_bFMODRunning = false;
	m_bLoadPlaylist = false;
	m_bVolumeDirty = true;
	m_iState = STATE_STOPPED;

	m_szPlayList[0] = '\0';
	// Start off with the default soundscape
	Q_strncpy( m_szSoundscape, GE_MUSIC_DEFAULTSCAPE, 64 );

	m_CurrPlaylist = NULL;

	m_fVolume = 1.0f;
	m_fNextSongTime = 0;
	m_fFadeStartTime = 0;
	m_iCurrSongLength = 0;

	m_iFadeMode = FADE_NONE;
	m_fLastFadeTime = 0;
	m_fFadeInPercent = 0;
	m_fFadeOutPercent = 0;

#ifdef _WIN32
	m_pLastSong = NULL;
	m_pCurrSong = NULL;

	// Setup the thread
	SetPriority( THREAD_PRIORITY_HIGHEST );
	SetName( "GEMusicMgr" );
#endif
}

CGEMusicManager::~CGEMusicManager()
{
	ClearPlayList();
	g_pGEMusicManager = NULL;
}

void CGEMusicManager::PostInit()
{
	ConVar *pVol = g_pCVar->FindVar( "snd_musicvolume" );
	pVol->InstallChangeCallback( GEMusicVolume_Callback );
	pVol->SetValue( pVol->GetFloat() );
}

void CGEMusicManager::LevelInitPreEntity()
{
	char levelname[64];
	Q_FileBase( engine->GetLevelName(), levelname, 64 );

	// Failsafe if the loading screen didn't set our music
	LoadPlayList( levelname );
}

void CGEMusicManager::LoadPlayList( const char *levelname )
{
	// Only load a new playlist if it is different than the current
	if ( Q_stricmp(levelname, m_szPlayList) )
	{
		Q_strncpy( m_szPlayList, levelname, sizeof(m_szPlayList) );
		m_bLoadPlaylist = true;
	}
}

void CGEMusicManager::ClearPlayList( void )
{
	// Clear out each vector
	for ( int i=m_Playlists.First(); i != m_Playlists.InvalidIndex(); i=m_Playlists.Next(i) )
		m_Playlists.Element( i )->PurgeAndDeleteElements();

	// Clear out the dictionary
	m_Playlists.PurgeAndDeleteElements();

	m_CurrPlaylist = NULL;
	m_nCurrSongIdx = -1;
}

void CGEMusicManager::SetSoundscape( const char *soundscape )
{
	m_Lock.Lock();

	Q_strncpy( m_szSoundscape, soundscape, 64 );
	m_bLoadSoundscape = true;

	m_Lock.Unlock();
}

void CGEMusicManager::SetVolume( float vol )
{
	m_fVolume = clamp( vol, 0, 1.0f );
	m_bVolumeDirty = true;
}

void CGEMusicManager::PauseMusic()
{
	if ( m_iState == STATE_PLAYING )
	{
		m_iState = STATE_PAUSED;
		StartFade( FADE_OUT );
		m_fPauseTime = CurrTime();
		DevMsg( 2, "[FMOD] Music Paused\n" );
	}
}

void CGEMusicManager::ResumeMusic()
{
	if ( m_iState == STATE_PAUSED )
	{
		if ( m_fNextSongTime > 0 )
			m_fNextSongTime += (CurrTime() - m_fPauseTime);

		m_iState = STATE_UNPAUSE;
		StartFade( FADE_IN );
		DevMsg( 2, "[FMOD] Music Resumed\n" );
	}
}

int CGEMusicManager::Run()
{
	m_bRun = true;

#ifdef _WIN32
	// Crate FMOD system
	FMOD_RESULT result = FMOD::System_Create( &m_pSystem );

	if ( result != FMOD_OK ) {
		Warning( "[FMOD] ERROR: System creation failed!\n" );
		return 0;
	} else {
		DevMsg( 2, "[FMOD] System successfully created\n" );
	}

	// Initialize FMOD system
	result = m_pSystem->init( 16, FMOD_INIT_NORMAL, 0 );

	if ( result != FMOD_OK ) {
		Warning( "[FMOD] ERROR: Failed to initialize!\n" );
		return 0;
	} else {
		DevMsg( 2, "[FMOD] Initialization successful\n" );
	}

	// Get our master channel group for volume control
	m_pSystem->getMasterChannelGroup( &m_pMasterChannel );

	m_bFMODRunning = true;

	while ( m_bRun && m_bFMODRunning )
	{
		// Set our volume if needed
		if ( m_bVolumeDirty )
		{
			m_pMasterChannel->setVolume( m_fVolume );
			m_bVolumeDirty = false;
		}

		// Load the playlist if we need to
		InternalLoadPlaylist();

		// Load the soundscape
		InternalLoadSoundscape();

		// Check if we need to go to the next song
		if ( m_iState != STATE_PAUSED && CurrTime() >= m_fNextSongTime )
			NextSong();

		// Check pause conditions
		PauseThink();

		// Check fade conditions
		FadeThink();

		// See if we are the current active window
		CheckWindowFocus();

		// Standard FMOD call
		m_pSystem->update();

		// Sleep for 20 ms for load balancing
		Sleep( 20 );
	}

	// Close down FMOD
	m_pMasterChannel->stop();
	EndSong( m_pLastSong );
	EndSong( m_pCurrSong );
	m_pSystem->release();
#endif // _WIN32

	return 0;
}

void CGEMusicManager::InternalLoadPlaylist( void )
{
	if ( !m_bLoadPlaylist )
		return;

	// Clear our playlist and stop playing music now
	ClearPlayList();

	// Only load a new list if we are given one
	if ( m_szPlayList[0] )
	{
		// Try to load the level's music definition
		char sz[128];
		Q_snprintf( sz, sizeof(sz), "scripts/music/level_music_%s", m_szPlayList );

		KeyValues *pKV = ReadEncryptedKVFile( filesystem, sz, NULL );
		if ( !pKV )
		{
			// If we fail to load the level music, try the default mix
			Q_snprintf( sz, sizeof(sz), "scripts/music/level_music_%s", GE_MUSIC_DEFAULT );
			pKV = ReadEncryptedKVFile( filesystem, sz, NULL );
		}

		if ( pKV )
		{
			// If we fail the default, don't play anything...
			m_Playlists.Insert( GE_MUSIC_DEFAULTSCAPE, new CUtlVector<char*>() );

			// Start the file parse
			KeyValues *pKey = pKV->GetFirstSubKey();
			while( pKey )
			{
				if ( !Q_stricmp( pKey->GetName(), "file" ) && !pKey->GetFirstSubKey() )
				{
					// A bare "file" key means add it to the default list
					AddSong( pKey->GetString(), GE_MUSIC_DEFAULTSCAPE );
				}
				else if ( pKey->GetFirstSubKey() )
				{
					// We encountered a soundscape entry
					const char *soundscape = pKey->GetName();

					KeyValues *pFile = pKey->GetFirstSubKey();
					while ( pFile )
					{
						if ( !Q_stricmp( pFile->GetName(), "file" ) )
							AddSong( pFile->GetString(), soundscape );
						pFile = pFile->GetNextKey();
					}
				}

				pKey = pKey->GetNextKey();
			}

			DevMsg( 2, "[FMOD] Loaded playlist %s\n", sz );
		}
	}

	m_fNextSongTime = 0;
	m_bLoadPlaylist = false;
}

void CGEMusicManager::EnforcePlaylist( bool force /*= false*/ )
{
	if ( !m_CurrPlaylist || force )
	{
		int idx = m_Playlists.Find( GE_MUSIC_DEFAULTSCAPE );
		if ( idx != m_Playlists.InvalidIndex() )
			m_CurrPlaylist = m_Playlists.Element( idx );
	}
}

void CGEMusicManager::InternalLoadSoundscape()
{
	if ( m_bLoadSoundscape )
	{
		m_Lock.Lock();

		// Cache this bad boy away
		CUtlVector<char*> *oldlist = m_CurrPlaylist;

		int idx = m_Playlists.Find( m_szSoundscape );
		if ( idx != m_Playlists.InvalidIndex() )
		{
			// Use the new playlist
			m_CurrPlaylist = m_Playlists.Element( idx );
			m_nCurrSongIdx = -1;

			DevMsg( 2, "[FMOD] Entering soundscape with playlist: %s", m_szSoundscape );
		}
		else
		{
			// Go back to default
			EnforcePlaylist( true );
		}

		// Load a song from the list if it changed
		if ( m_CurrPlaylist != oldlist )
			NextSong();

		m_bLoadSoundscape = false;

		m_Lock.Unlock();
	}
}

void CGEMusicManager::NextSong( void ) 
{
#ifdef _WIN32
	// Make sure we are not NULL...
	EnforcePlaylist();
	if ( !m_CurrPlaylist )
		return;

	FMOD_RESULT result;
	int num_songs = m_CurrPlaylist->Count();
	if ( num_songs == 0 )
		return;

	int idx = 0;
	if ( num_songs > 1 )
	{
		// Find us a random song index not including our last song
		do
		{
			idx = GERandom<int>( num_songs );
		} while ( idx == m_nCurrSongIdx );
	}

	const char *song_entry = m_CurrPlaylist->Element( idx );

	char soundfile[1024];
	Q_snprintf( soundfile, sizeof(soundfile), "%s/sound/%s", engine->GetGameDirectory(), song_entry );
	Q_FixSlashes( soundfile );

	// Load the new song with FMOD
	FMOD::Sound *pSound;
	result = m_pSystem->createStream( soundfile, FMOD_DEFAULT, 0, &pSound );
	if ( result != FMOD_OK )
	{
		Warning( "[FMOD] Failed to create sound stream '%s'! Error Code: %i\n", song_entry, result );
		return;
	}

	DevMsg( 2, "[FMOD] Loaded song %s\n", song_entry );

	// Make the current song the last song for fading purposes, make sure the last song is stopped
	if ( m_pLastSong )
		EndSong( m_pLastSong );

	m_pLastSong = m_pCurrSong;

	// Load the new sound as the current song
	result = m_pSystem->playSound( pSound, m_pMasterChannel, true, &m_pCurrSong );
	if ( result != FMOD_OK )
	{
		Warning( "[FMOD] Failed to play sound stream '%s'! Error Code: %i\n", song_entry, result );
		return;
	}

	m_pCurrSong->setVolume( 0 );
	m_pCurrSong->setPaused( false );

	// Record our current song choice index and length of play
	pSound->getLength( &m_iCurrSongLength, FMOD_TIMEUNIT_MS );
	
	m_fNextSongTime = CurrTime() - FMOD_TRANSITION_TIME + (m_iCurrSongLength / 1000);
	m_nCurrSongIdx  = idx;
	m_iState		= STATE_PLAYING;

	// Start fading us in
	StartFade( FADE_IN );

	DevMsg( 2, "[FMOD] Playing song %s\n", song_entry );
#endif // _WIN32
}

void CGEMusicManager::CheckWindowFocus( void )
{
	static bool sbInFocus = true;

	// If we are on windows (why wouldn't we be?) check if we are the active window
#ifdef _WIN32
	if ( m_pMasterChannel )
	{
		HWND active = GetForegroundWindow();
		if ( active == gWindowHandle && !sbInFocus ) {
			m_pMasterChannel->setMute( false );
			sbInFocus = true;
		} else if ( active != gWindowHandle && sbInFocus ) {
			m_pMasterChannel->setMute( true );
			sbInFocus = false;
		}
	}
#endif
}

void CGEMusicManager::StartFade( int type )
{
#ifdef _WIN32
	if ( m_iFadeMode != FADE_NONE && m_iFadeMode != type )
	{
		// Changed in-progress fade, swap status
		m_fFadeInPercent = m_fFadeOutPercent;
	}
	else if ( m_iFadeMode != type )
	{
		// Fresh fade, reset our status
		m_fFadeInPercent = 0;
		m_fFadeOutPercent = 1.0f;
	}

	m_iFadeMode = type;
	m_fFadeStartTime = CurrTime();
	m_fLastFadeTime = m_fFadeStartTime;
#endif
}

void CGEMusicManager::FadeThink( void )
{
#ifdef _WIN32
	// Sanity Check
	if ( m_fLastFadeTime < m_fFadeStartTime )
		return;

	float now = CurrTime();
	float fade_delta = (now - m_fLastFadeTime) / FMOD_TRANSITION_TIME;
	m_fLastFadeTime = now;

	// Linear fade
	m_fFadeInPercent  = min( m_fFadeInPercent + fade_delta, 1.0f );
	m_fFadeOutPercent = max( m_fFadeOutPercent - fade_delta, 0 );

	// Fail-safe
	if ( now > m_fFadeStartTime + FMOD_TRANSITION_TIME )
	{
		m_fFadeInPercent = 1.0f;
		m_fFadeOutPercent = 0;
	}
	
	if ( m_iFadeMode == FADE_IN )
	{
		// m_pLastSong OUT, m_pCurrSong IN
		if ( m_pLastSong )
			m_pLastSong->setVolume( m_fFadeOutPercent );
		
		if ( m_pCurrSong )
			m_pCurrSong->setVolume( m_fFadeInPercent );
	}
	else if ( m_iFadeMode == FADE_OUT )
	{
		// m_pLastSong OUT, m_pCurrSong OUT
		if ( m_pLastSong )
			m_pLastSong->setVolume( m_fFadeOutPercent );

		if ( m_pCurrSong )
			m_pCurrSong->setVolume( m_fFadeOutPercent );
	}

	// Check if we finished fading
	if ( m_iFadeMode != FADE_NONE && m_fFadeInPercent >= 1.0f && m_fFadeOutPercent <= 0 )
	{
		DevMsg( 2, "[FMOD] Fade %s completed\n", m_iFadeMode == FADE_IN ? "IN" : "OUT" );

		m_iFadeMode = FADE_NONE;

		// Stop our last song
		if ( m_pLastSong )
		{
			EndSong( m_pLastSong );
			m_pLastSong = NULL;
		}
	}
#endif // _WIN32
}

void CGEMusicManager::PauseThink()
{
#ifdef _WIN32
	if ( !m_pCurrSong )
		return;

	if ( m_iState == STATE_UNPAUSE )
	{
		// Set our next song time based on our current position in the song
		if ( m_fNextSongTime > 0 )
		{
			unsigned int currPos = 0;
			m_pCurrSong->getPosition( &currPos, FMOD_TIMEUNIT_MS );
			m_fNextSongTime = CurrTime() - FMOD_TRANSITION_TIME + ((m_iCurrSongLength - currPos) / 1000);
		}

		// Unpause and set our state to playing
		m_pCurrSong->setPaused( false );
		m_iState = STATE_PLAYING;
	}
	else if ( m_iState == STATE_PAUSED )
	{
		// Pause our song if we are done fading
		if ( m_iFadeMode == FADE_NONE )
			m_pCurrSong->setPaused( true );
	}
#endif // _WIN32
}

#ifdef _WIN32
void CGEMusicManager::EndSong( FMOD::Channel *pChannel )
{
	if ( !pChannel )
		return;

	// Release the sound (NOTE: the channel is "stopped" automatically)
	Sound *pSound;
	FMOD_RESULT r = pChannel->getCurrentSound( &pSound );
	if ( r == FMOD_OK )
		pSound->release();
	else
		DevMsg( 2, "[FMOD] Failed to release sound! Error Code: %i\n", r );
}
#endif

void CGEMusicManager::AddSong( const char *relative, const char *soundscape )
{
	// See if we need to add the soundscape
	int idx = m_Playlists.Find( soundscape );
	if ( idx == m_Playlists.InvalidIndex() )
		idx = m_Playlists.Insert( soundscape, new CUtlVector<char*>() );

	// Get the soundscape list
	CUtlVector<char*> *list = m_Playlists.Element( idx );

	// Check if we currently have this song in our list
	for ( int i = 0 ; i < list->Count() ; ++i )
	{
		// Don't add a song if it already exists in the list
		if ( !Q_stricmp( list->Element( i ), relative ) )
			return;
	}

	// Allocate new space for this song
	int len = min( Q_strlen(relative) + 1, 64 );
	char *song = new char[len];
	Q_strncpy( song, relative, len );

	list->AddToTail( song );
}

void CGEMusicManager::DEBUG_NextSong()
{
	m_fNextSongTime = 0;
}

// This could actually be useful for players/musicians testing their tracks.
CON_COMMAND( ge_nextsong, "Skips current track" )
{
	GEMusicManager()->DEBUG_NextSong();
}

#ifdef _DEBUG
CON_COMMAND( fmod_playlist, "Playlist change" )
{
	if ( args.ArgC() > 1 )
	{
		GEMusicManager()->LoadPlayList( args[1] );
	}
}

CON_COMMAND( fmod_pause, "Pauses music" )
{
	GEMusicManager()->PauseMusic();
}

CON_COMMAND( fmod_resume, "Resumes music" )
{
	GEMusicManager()->ResumeMusic();
}
#endif
