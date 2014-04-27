//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GE_CLIENTMODE_H
#define GE_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;

namespace vgui
{
	typedef unsigned long HScheme;
}

struct GameplayTip
{
	GameplayTip() { name[0] = line1[0] = line2[0] = line3[0] = L'\0'; }
	wchar_t name[64];
	char line1[64];
	char line2[64];
	char line3[64];
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ClientModeGENormal : public ClientModeShared
{
public:
	DECLARE_CLASS( ClientModeGENormal, ClientModeShared );

	ClientModeGENormal();
	~ClientModeGENormal();

	virtual void	Init();

	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );

	virtual int		GetDeathMessageStartHeight( void );

	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual void	OverrideMouseInput( float *x, float *y );

	virtual bool	QueuePanel( const char *panelName, KeyValues *data = NULL );

	virtual void LoadGameplayTips();
	virtual void UnloadGameplayTips();
	virtual const GameplayTip *GetRandomTip();
	virtual const char *GetRandomTipLine( const GameplayTip *tip );

	virtual CBaseViewport *GetBaseViewport( void ) { return m_pViewport; };

private:
	CUtlVector<GameplayTip*> m_vGameplayTips;
	int m_iLastTip;
};

extern IClientMode *GetClientModeNormal();
extern ClientModeGENormal* GetClientModeGENormal();

#endif // GE_CLIENTMODE_H
