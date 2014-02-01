///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_player_shared.h
// Description:
//      see ge_player_shared.cpp
//
// Created On: 23 Feb 08, 17:20
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_PLAYER_SHARED_H
#define GE_PLAYER_SHARED_H
#pragma once

#define GE_PUSHAWAY_THINK_INTERVAL		(1.0f / 20.0f)
#include "studio.h"

#if defined( CLIENT_DLL )
	#define CGEPlayer C_GEPlayer
	#define CGEMPPlayer C_GEMPPlayer
	#define CGEBotPlayer C_GEBotPlayer
#endif

class C_GEPlayer;

enum {
	AIM_NONE,
	AIM_ZOOM_IN,
	AIM_ZOOMED,
	AIM_ZOOM_OUT
};

#define WEAPON_ZOOM_RATE 180.0f

#endif //GE_PLAYER_SHARED_h

