///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudchat.h
// Description:
//      HUD Chat
//
// Created On: 08 Nov 08
// Created By: Jonathan White "killermonkey" 
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_HUD_CHAT_H
#define GE_HUD_CHAT_H
#ifdef _WIN32
#pragma once
#endif

#include "hud_basechat.h"
#include "ge_hudgameplay.h"

class CHudChatLine : public CBaseHudChatLine
{
	DECLARE_CLASS_SIMPLE( CHudChatLine, CBaseHudChatLine );

public:
	CHudChatLine( vgui::Panel *parent, const char *panelName ) : CBaseHudChatLine( parent, panelName ) {}
	void			MsgFunc_SayText(bf_read &msg);

private:
	CHudChatLine( const CHudChatLine & ); // not defined, not accessible
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
class CHudChatInputLine : public CBaseHudChatInputLine
{
	DECLARE_CLASS_SIMPLE( CHudChatInputLine, CBaseHudChatInputLine );
	
public:
	CHudChatInputLine( CBaseHudChat *parent, char const *panelName ) : CBaseHudChatInputLine( parent, panelName ) {}
};

class CHudChat : public CBaseHudChat
{
	DECLARE_CLASS_SIMPLE( CHudChat, CBaseHudChat );

public:
	CHudChat( const char *pElementName );

	virtual void	CreateChatInputLine( void );
	virtual void	CreateChatLines( void );

	virtual void	OnTick( void );
	virtual void	Init( void );
	virtual void	Reset( void );

	virtual bool	ShouldDraw(void);

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );

	int				GetChatInputOffset( void );

	virtual Color	GetClientColor( int clientIndex );

private:
	vgui::IScheme	*m_IScheme;
	CHudGameplay	*m_pHudGameplay;
	bool m_bIntermissionSet;
};

#endif	//GE_HUD_CHAT_H
