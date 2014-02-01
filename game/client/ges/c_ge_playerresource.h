///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_playerresource.h
// Description:
//      Collects each player's GE specific information from server
//
// Created On: 15 Sep 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef C_GEPLAYERRESOURCE_H
#define C_GEPLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "c_playerresource.h"
#include "GameEventListener.h"

class C_GEPlayerResource : public C_PlayerResource
{
	DECLARE_CLASS( C_GEPlayerResource, C_PlayerResource );
public:
	DECLARE_CLIENTCLASS();

	C_GEPlayerResource();
	~C_GEPlayerResource();

	virtual const char* GetCharIdent( int index );
	virtual const char*	GetCharName( int index );
	virtual const int GetFavoriteWeapon( int index );
	virtual const int GetDevStatus( int index );
	virtual const int GetCampingPercent( int index );
	virtual const int GetScoreBoardColor( int index );
	virtual int GetFrags( int index );
	virtual const bool IsActive( int index );

protected:
	char	m_szCharIdent		[MAX_PLAYERS+1][32];
	int		m_iFavWeapon		[MAX_PLAYERS+1];
	int		m_iDevStatus		[MAX_PLAYERS+1];
	int		m_iCampingPercent	[MAX_PLAYERS+1];
	int		m_iFrags			[MAX_PLAYERS+1];
	int		m_iScoreBoardColor	[MAX_PLAYERS+1];
	bool	m_bIsActive			[MAX_PLAYERS+1];
};

extern C_GEPlayerResource *GEPlayerRes();

#endif
