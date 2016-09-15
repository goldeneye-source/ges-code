///////////// Copyright © 2008, Anthony Iacono. All rights reserved. /////////////
// 
// File: ge_hudlifebars.h
// Description:
//      Goldeneye style health and armor bar.
//
// Created On: 03/07/2008
// Created By: Anthony Iacono
/////////////////////////////////////////////////////////////////////////////

#include "hudelement.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/Controls.h"

using namespace vgui;

class CGEHudLifebars : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CGEHudLifebars, vgui::Panel );

public:
	CGEHudLifebars( const char *pElementName );
	void Init( void );
	void OnThink( void );
	void MsgFunc_Battery( bf_read &msg );
	void MsgFunc_Damage( bf_read &msg );
	void MsgFunc_HideLifeBars( bf_read &msg );
	void LoadBars( void );
	void DrawBars( void );

protected:
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint();

private:
	float CalcDialPoints( float curr, float max );

	CHudTexture		*m_pHealthbarFull;
	CHudTexture		*m_pHealthbarEmpty;
	CHudTexture		*m_pArmorbarFull;
	CHudTexture		*m_pArmorbarEmpty;

	bool		m_bInFlash;
	float		m_flFlashResetTime;

	bool		m_bBarsFlashing;
	float		m_flHideBarsTime;

	vgui::AnimationController	*m_pAnimController;

	int m_Health;
	float m_Armor;
	int m_HealthPercent;
	int m_ArmorPercent;
};
