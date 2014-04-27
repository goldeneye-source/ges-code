///////////// Copyright © 2010, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_bloodscreenvm.h
// Description:
//      Special view model for the bloodscreen. Animations are completely controlled
//		client side unless being viewed as a specator
//
// Created On: 24 Jan 2010
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_BLOODSCREENVM_H
#define GE_BLOODSCREENVM_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"

#if defined( CLIENT_DLL )
#define CGEBloodScreenVM C_GEBloodScreenVM
#endif

class CGEBloodScreenVM : public CBaseViewModel
{
	DECLARE_CLASS( CGEBloodScreenVM, CBaseViewModel );
public:

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CGEBloodScreenVM( void ) {};
	
	virtual void		CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles );

	virtual void		Spawn( void );

#if !defined( CLIENT_DLL )
	virtual int				UpdateTransmitState( void );
#else
	virtual bool			ShouldDraw();
#endif
};

#endif
