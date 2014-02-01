///////////// Copyright � 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudprogressbars.cpp
// Description:
//      Shows the local player's gameplay progress via bars and/or text
//
// Created On: 06 Jan 10
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_HUDPROGRESSBARS
#define GE_HUDPROGRESSBARS

#include "hud.h"
#include "hudelement.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ProgressBar.h"

#define GE_HUD_NUMPROGRESSBARS 8

struct sProgressBar
{
	float currValue;
	float maxValue;
	int flags;

	bool visible;

	// Specifics for drawing (NOT ADJUSTED!)
	float x, y;
	int w, h;

	vgui::Label *pTitle;
	vgui::Label *pValue;
	vgui::ContinuousProgressBar *pBar;
};

class CHudProgressBars : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudProgressBars, vgui::Panel );

public:
	CHudProgressBars( const char *pElementName );
	~CHudProgressBars();
	virtual bool ShouldDraw();
	virtual void VidInit();

	virtual void FireGameEvent( IGameEvent * event );

	void MsgFunc_AddProgressBar( bf_read &msg );
	void MsgFunc_RemoveProgressBar( bf_read &msg );
	void MsgFunc_UpdateProgressBar( bf_read &msg );
	void MsgFunc_ConfigProgressBar( bf_read &msg );

protected:
	virtual void PerformLayout();
	virtual void ResetProgressBar( int id );
	virtual void LayoutProgressBar( int id );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	vgui::AnimationController *m_pHudAnims;

	// Keep track of our own progress bars only
	sProgressBar *m_vProgressBars[GE_HUD_NUMPROGRESSBARS];

	CPanelAnimationVar( vgui::HFont, m_hFont, "font", "Default" );
};

#endif
