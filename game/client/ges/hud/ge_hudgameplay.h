///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudgameplay.cpp
// Description:
//      Gameplay information HUD
//
// Created On: 06/20/2009
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_HUDGAMEPLAY
#define GE_HUDGAMEPLAY

#include "hud.h"
#include "hudelement.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Label.h"
#include "ge_CreditsText.h"
#include "ge_VertListPanel.h"
#include "c_ge_gameplayresource.h"

//-----------------------------------------------------------------------------
// Purpose: Display in-game information regarding gameplay and weaponset
//-----------------------------------------------------------------------------
class CHudGameplay : public vgui::Panel, public CHudElement, public C_GEGameplayResourceListener
{
	DECLARE_CLASS_SIMPLE(CHudGameplay, vgui::Panel);

public:
	CHudGameplay(const char *pElementName);
	~CHudGameplay();

	virtual void Reset();
	virtual void VidInit();

	virtual void OnGameplayDataUpdate();

	virtual void PerformLayout();
	virtual bool RequestInfo(KeyValues *outputData);
	virtual void ResolveWeaponHelp();
	inline void InsertTokenWithValue(const char* token, float value, bool precise = true);
	inline void InsertTokenWithBar(const char* token, int length);

	virtual bool IsRaised(void) { return m_flHeightPercent >= 0.95f; };

	void KeyboardInput(int action);

	enum {
		GAMEPLAY_HIDDEN = 0,
		GAMEPLAY_WEAPONSET,
		GAMEPLAY_HELP,
		GAMEPLAY_STATS,
		GAMEPLAY_WEAPONSTATS,

		MOVEMENT_SHOWING,
		MOVEMENT_HIDING,
		MOVEMENT_NONE,
	};

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnThink();

	virtual void ResolveGameplayHelp();

private:
	void PaintString(const wchar_t *text, Color col, vgui::HFont& font, int x, int y);

	int m_iCurrState;
	int m_iCurrMovement;
	int m_iOrigX;
	int m_iOrigY;
	int m_iCurrWepID;

	vgui::AnimationController *m_pHudAnims;

	vgui::ImagePanel *m_pBackground;

	// Weapon Set Panel
	struct sWeaponLabel
	{
		sWeaponLabel(void) { pLabel = NULL; pImage = NULL; };
		void SetPos(int x, int y) {
			pLabel->SetPos(x, y);
			pImage->SetPos(x + pLabel->GetWide(), y - pLabel->GetTall() / 3);
		};
		int GetWide(void) { return pLabel->GetWide() + pImage->GetWide(); };
		int GetTall(void) { return max(pLabel->GetTall(), pImage->GetTall()); };

		vgui::Label	*pLabel;
		vgui::ImagePanel *pImage;
	};

	vgui::Panel					*m_pWeaponSet;
	vgui::Label					*m_pWeaponSetName;
	CUtlVector<sWeaponLabel*>	m_vWeaponLabels;

	// Gameplay Help Panel
	vgui::GECreditsText	*m_pGameplayHelp;

	// Weapon Help Panel
	vgui::GECreditsText	*m_pWeaponHelp;

	// Configurable Parameters
	CPanelAnimationVarAliasType(int, m_iTabWidth, "tab_width", "16", "proportional_int");
	CPanelAnimationVarAliasType(int, m_iTextX, "text_xpad", "8", "proportional_int");
	CPanelAnimationVarAliasType(int, m_iTextY, "text_ypad", "8", "proportional_int");

	CPanelAnimationVar(vgui::HFont, m_hWeaponFont, "weapon_font", "Default");
	CPanelAnimationVar(vgui::HFont, m_hWeaponSymbolFont, "weapon_symbol_font", "Default");
	CPanelAnimationVar(vgui::HFont, m_hHelpFont, "help_font", "Default");
	CPanelAnimationVar(vgui::HFont, m_hHeaderFont, "header_font", "Default");
	CPanelAnimationVar(Color, m_TextColor, "fgcolor", "0 0 0 255");
	CPanelAnimationVar(Color, m_BgColor, "bgcolor", "0 0 0 100");

	// Define these to control specific parts of the HUD via a scripted animation
	CPanelAnimationVar(float, m_flHeightPercent, "HeightPercent", "0");
};

#endif
