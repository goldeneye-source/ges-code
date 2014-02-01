///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
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
#ifndef GETARGETID
#define GETARGETID
#include "hud.h"
#include "hudelement.h"
#include "materialsystem/imaterial.h"

using namespace vgui;

class CGETargetID : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CGETargetID, vgui::Panel );

public:
	CGETargetID( const char *pElementName );
	~CGETargetID();

	void Init( void ) { }
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void VidInit( void );

	void DrawOverheadIcons();

private:
	Color			GetColorForTargetTeam( int iTeamNumber );
	vgui::HFont		m_hFont;
	int				m_iLastEntIndex;
	float			m_flLastChangeTime;

	IMaterial*		m_pJanusIcon;
	IMaterial*		m_pMI6Icon;
};

#endif