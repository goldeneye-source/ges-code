///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudgameplay.cpp
// Description:
//      See Header
//
// Created On: 01 Jul 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_ge_player.h"
#include "c_ge_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "gemp_gamerules.h"
#include "c_team.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/TextImage.h"
#include "ge_hudgameplay.h"
#include "networkstringtable_clientdll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH(CHudGameplay, 100);

#define HUDGP_SETICONDIR "hud/weaponseticons/"
#define HUDWP_BARLENGTH 96

ConVar cl_ge_hud_advancedweaponstats("cl_ge_hud_advancedweaponstats", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Show more detailed weapon stats in the popout panel.");

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudGameplay::CHudGameplay(const char *pElementName) : BaseClass(NULL, "GEHUDGameplay"), CHudElement(pElementName)
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_pHudAnims = g_pClientMode->GetViewportAnimationController();

	m_iCurrState = GAMEPLAY_HIDDEN;
	m_iCurrMovement = MOVEMENT_NONE;
	m_flHeightPercent = 0;
	m_iCurrWepID = -1;

	m_pBackground = new vgui::ImagePanel(this, "Background");
	m_pBackground->SetImage("hud/GameplayHUDBg");
	m_pBackground->SetShouldScaleImage(true);
	m_pBackground->SetPaintBackgroundType(1);
	m_pBackground->SetPaintBorderEnabled(false);

	m_pWeaponSet = new vgui::Panel(this, "WeaponSet");
	m_pWeaponSet->SetPaintBackgroundEnabled(false);
	m_pWeaponSet->SetPaintBorderEnabled(false);

	m_pWeaponSetName = new vgui::Label(m_pWeaponSet, "weaponsetname", "NULL");
	m_pWeaponSetName->SetPaintBackgroundEnabled(false);
	m_pWeaponSetName->SetPaintBorderEnabled(false);

	char name[8];
	for (int i = 0; i < MAX_WEAPON_SPAWN_SLOTS; i++)
	{
		Q_snprintf(name, 8, "slot%i", i);
		m_vWeaponLabels.AddToTail(new sWeaponLabel);
		m_vWeaponLabels.Tail()->pImage = new vgui::ImagePanel(m_pWeaponSet, name);
		m_vWeaponLabels.Tail()->pImage->SetShouldScaleImage(true);
		m_vWeaponLabels.Tail()->pLabel = new vgui::Label(m_pWeaponSet, name, "");
	}

	m_pGameplayHelp = new vgui::GECreditsText(this, "GameplayHelp");
	m_pGameplayHelp->SetPaintBackgroundEnabled(false);
	m_pGameplayHelp->SetPaintBorderEnabled(false);
	m_pGameplayHelp->SetVerticalScrollbar(false);


	m_pWeaponHelp = new vgui::GECreditsText(this, "WeaponHelp");
	m_pWeaponHelp->SetPaintBackgroundEnabled(false);
	m_pWeaponHelp->SetPaintBorderEnabled(false);
	m_pWeaponHelp->SetVerticalScrollbar(false);
}

CHudGameplay::~CHudGameplay()
{
	m_vWeaponLabels.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the HUD element on map change
//-----------------------------------------------------------------------------
void CHudGameplay::Reset()
{
	if (m_iCurrMovement != MOVEMENT_NONE)
	{
		if (m_iCurrMovement == MOVEMENT_SHOWING)
			m_flHeightPercent = 1.0f;
		else
			m_flHeightPercent = 0;
	}
}

void CHudGameplay::VidInit()
{
	// Make sure we hide our goods
	m_iCurrState = GAMEPLAY_HIDDEN;

	// Explicitly reset our ypos to 0% height
	m_flHeightPercent = 0;

	SetZPos(3);

	InvalidateLayout();
	Reset();
}

bool CHudGameplay::RequestInfo(KeyValues *outputData)
{
	if (!stricmp(outputData->GetName(), "HeightPercent"))
	{
		outputData->SetFloat("HeightPercent", m_flHeightPercent);
		return true;
	}

	return BaseClass::RequestInfo(outputData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGameplay::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(Color(0, 0, 0, 0));

	m_pBackground->SetBgColor(m_BgColor);
	m_pBackground->SetFgColor(GetFgColor());

	m_pWeaponSetName->SetFgColor(m_TextColor);
	m_pWeaponSetName->SetFont(m_hWeaponFont);

	m_pGameplayHelp->SetFgColor(m_TextColor);
	m_pGameplayHelp->SetBgColor(m_BgColor);
	m_pGameplayHelp->SetFont(m_hWeaponFont);
	m_pGameplayHelp->SetWide(GetWide());

	GetPos(m_iOrigX, m_iOrigY);

	SetZPos(500);
	SetAlpha(255);
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
}

void CHudGameplay::OnGameplayDataUpdate(void)
{
	char szClassName[32], szTexture[256];
	wchar_t szPrintName[32], *szName;

	// Set the weaponset name
	szName = g_pVGuiLocalize->Find(GEGameplayRes()->GetLoadoutName());
	if (szName)
		m_pWeaponSetName->SetText(szName);
	else
		m_pWeaponSetName->SetText(GEGameplayRes()->GetLoadoutName());
	m_pWeaponSetName->SetFont(m_hHeaderFont);
	m_pWeaponSetName->SizeToContents();

	// Set the weapons
	CUtlVector<int> weapons;
	GEGameplayRes()->GetLoadoutWeaponList(weapons);

	for (int i = 0; i < weapons.Count(); i++)
	{
		szName = g_pVGuiLocalize->Find(GetWeaponPrintName(weapons[i]));
		if (szName)
			_snwprintf(szPrintName, 32, L"%i. %s", i + 1, szName);
		else
			_snwprintf(szPrintName, 32, L"%i. Unknown Weapon", i + 1);

		m_vWeaponLabels[i]->pLabel->SetText(szPrintName);
		m_vWeaponLabels[i]->pLabel->SetFgColor(m_TextColor);
		m_vWeaponLabels[i]->pLabel->SetFont(m_hWeaponFont);
		m_vWeaponLabels[i]->pLabel->SizeToContents();

		Q_strncpy(szClassName, WeaponIDToAlias(weapons[i]), 32);
		Q_StrRight(szClassName, strlen(szClassName) - 7, szClassName, 32);
		Q_snprintf(szTexture, 256, "%s%s", HUDGP_SETICONDIR, szClassName);
		m_vWeaponLabels[i]->pImage->SetImage(szTexture);
		m_vWeaponLabels[i]->pImage->SetSize(YRES(64), YRES(16)); // Use YRES for both so that the image doesn't stretch on widescreen resolutions.
	}

	ResolveGameplayHelp();
	InvalidateLayout();
}

void CHudGameplay::ResolveGameplayHelp(void)
{
	if (!g_pStringTableGameplay)
		return;

	m_pGameplayHelp->SetText("");
	m_pGameplayHelp->InsertCenterText(true);
	m_pGameplayHelp->InsertFontChange(m_hHeaderFont);

	m_pGameplayHelp->InsertString(GEGameplayRes()->GetGameplayName());

	m_pGameplayHelp->InsertCenterText(false);
	m_pGameplayHelp->InsertFontChange(m_hHelpFont);
	m_pGameplayHelp->InsertString("\n\n");

	const char *pHelpString = g_pStringTableGameplay->GetString(GEGameplayRes()->GetGameplayHelp());
	if (pHelpString)
	{
		m_pGameplayHelp->InsertString(pHelpString);
	}
	else
	{
		wchar_t *szParsedHelp = g_pVGuiLocalize->Find("#GES_GP_NOHELP");
		if (szParsedHelp)
			m_pGameplayHelp->InsertString(szParsedHelp);
		else
			m_pGameplayHelp->InsertString("No help available for this gameplay");
	}
}

void CHudGameplay::ResolveWeaponHelp(void)
{
	CGEPlayer *pGEPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());

	if (!pGEPlayer)
		return;

	CGEWeapon *pGEWeapon = ToGEWeapon(pGEPlayer->GetActiveWeapon());

	if (!pGEWeapon)
		return;

	m_iCurrWepID = pGEWeapon->GetWeaponID();

	m_pWeaponHelp->SetText("");
	m_pWeaponHelp->InsertCenterText(true);
	m_pWeaponHelp->InsertFontChange(m_hHeaderFont);

	m_pWeaponHelp->InsertString(pGEWeapon->GetPrintName());

	m_pWeaponHelp->InsertCenterText(false);
	m_pWeaponHelp->InsertFontChange(m_hWeaponFont);
	m_pWeaponHelp->InsertString("\n\n");
	
	// We consider basedamage bodyshot damage usually.
	// We want everything to be bonuses instead of penalties though, so if bodyshot damage is higher than normal the basedamage will be lower
	// so we can have a "bodyshot damage bonus" on the weapon

	int basedamage = pGEWeapon->GetWeaponDamage();

	if (pGEWeapon->IsShotgun())
		basedamage *= 5;

	int headshotdamage = basedamage * pGEWeapon->GetHitBoxDataModifierHead();
	int bodyshotdamage = basedamage * pGEWeapon->GetHitBoxDataModifierChest();
	int limbshotdamage = basedamage * (pGEWeapon->GetHitBoxDataModifierRightLeg() + pGEWeapon->GetHitBoxDataModifierRightArm()) * 0.5;

	float RPS = min(1 / (pGEWeapon->GetClickFireRate() + pGEWeapon->GetFireDelay()), pGEWeapon->GetMaxClip1() == -1 ? 100 : pGEWeapon->GetMaxClip1());

	if (cl_ge_hud_advancedweaponstats.GetBool())
	{
		if (!pGEWeapon->IsExplosiveWeapon())
		{
			InsertTokenWithValue("#GE_WH_DPHS", pGEWeapon->GetDamageCap() * 0.05);
			InsertTokenWithValue("#GE_WH_HeadDamage", headshotdamage * 0.05);
			InsertTokenWithValue("#GE_WH_ChestDamage", bodyshotdamage * 0.05);
			InsertTokenWithValue("#GE_WH_LimbDamage", limbshotdamage * 0.05);
		}
		else
		{
			InsertTokenWithValue("#GE_WH_Damage", basedamage * 0.05);
			InsertTokenWithValue("#GE_WH_Radius", pGEWeapon->GetGEWpnData().m_flDamageRadius / 12);
		}
	}
	else
	{
		float units = (MAX_HEALTH * 2) / HUDWP_BARLENGTH;

		if (!pGEWeapon->IsExplosiveWeapon())
		{
			InsertTokenWithBar("#GE_WH_DPHSGraph", round(min(pGEWeapon->GetDamageCap(), RPS * headshotdamage) / units));
			InsertTokenWithBar("#GE_WH_HeadDamageGraph", round(headshotdamage / units));
			InsertTokenWithBar("#GE_WH_ChestDamageGraph", round(bodyshotdamage / units));
			InsertTokenWithBar("#GE_WH_LimbDamageGraph", round(limbshotdamage / units));
		}
		else
		{
			InsertTokenWithBar("#GE_WH_DamageGraph", round(basedamage * HUDWP_BARLENGTH / 528));
			InsertTokenWithBar("#GE_WH_RadiusGraph", round(pGEWeapon->GetGEWpnData().m_flDamageRadius * HUDWP_BARLENGTH/140));
		}
	}


	if (!pGEWeapon->IsMeleeWeapon() && pGEWeapon->GetWeaponID() != WEAPON_GRENADE) // Grenades have the special throwing behavior which mucks up this calculation.
		InsertTokenWithValue("#GE_WH_RPS", RPS);

	// Explosive and melee weapons don't have any spread.
	if (!pGEWeapon->IsMeleeWeapon() && !pGEWeapon->IsExplosiveWeapon())
	{
		wchar_t szBuf[128];
		wchar_t *wszMsg;

		const char *qArray[8] = { "#GE_Q_None", "#GE_Q_VeryLow", "#GE_Q_Low", "#GE_Q_Average", "#GE_Q_High", "#GE_Q_VeryHigh", "#GE_Q_VVHigh", "#GE_Q_VVVHigh" };

		float basespread = min(pGEWeapon->GetGEWpnData().m_vecSpread.x + 1, 7);

		// If we read exactly 1 basespread it means we really had no spread at all.  This gets a special catagory.
		if (basespread == 1)
			basespread = 0;

		int spreadpos = round(basespread / sqrt(min(pGEWeapon->GetGEWpnData().m_flGaussFactor*0.3, 1) + 1));

		wszMsg = g_pVGuiLocalize->Find("#GE_WH_MinSpread");
		g_pVGuiLocalize->ConstructString(szBuf, sizeof(szBuf), wszMsg, 1, g_pVGuiLocalize->Find(qArray[spreadpos]));
		m_pWeaponHelp->InsertString(szBuf);
		m_pWeaponHelp->InsertString("\n");

		float maxspread = min(pGEWeapon->GetGEWpnData().m_vecMaxSpread.x + 1, 7);

		// If we read exactly 1 maxspread it means we really had no spread at all.  This gets a special catagory.
		if (maxspread == 1)
			maxspread = 0;

		spreadpos = round(maxspread / sqrt(min(pGEWeapon->GetGEWpnData().m_flGaussFactor*0.3, 1) + 1));

		wszMsg = g_pVGuiLocalize->Find("#GE_WH_MaxSpread");
		g_pVGuiLocalize->ConstructString(szBuf, sizeof(szBuf), wszMsg, 1, g_pVGuiLocalize->Find(qArray[spreadpos]));
		m_pWeaponHelp->InsertString(szBuf);
		m_pWeaponHelp->InsertString("\n");
	}

	m_pWeaponHelp->InsertString("\n");

	// Special attributes

	if (pGEWeapon->GetMaxPenetrationDepth() > 0)
	{
		InsertTokenWithValue("#GE_Att_Penetrate", floor(pGEWeapon->GetMaxPenetrationDepth()), false);
	}

	if (pGEWeapon->GetGEWpnData().m_flAimBonus > 0)
	{
		m_pWeaponHelp->InsertString(g_pVGuiLocalize->Find("#GE_Att_AimBoost"));
		m_pWeaponHelp->InsertString("\n");
	}

	if (pGEWeapon->IsSilenced())
	{
		m_pWeaponHelp->InsertString(g_pVGuiLocalize->Find("#GE_Att_Tracers"));
		m_pWeaponHelp->InsertString("\n");
	}

	if (Q_strcmp(pGEWeapon->GetSpecAttString(), ""))
		m_pWeaponHelp->InsertString(g_pVGuiLocalize->Find(pGEWeapon->GetSpecAttString()));
}


inline void CHudGameplay::InsertTokenWithValue(const char* token, float value, bool precise)
{
	static char strBuffer[128];
	static wchar_t szBuf[128];
	static wchar_t szValBuf[128];
	static wchar_t *wszMsg;

	if (precise)
		Q_snprintf(strBuffer, sizeof(strBuffer), "%0.1f", value);
	else
		Q_snprintf(strBuffer, sizeof(strBuffer), "%0.0f", value);

	g_pVGuiLocalize->ConvertANSIToUnicode(strBuffer, szValBuf, sizeof(szValBuf));

	wszMsg = g_pVGuiLocalize->Find(token);
	g_pVGuiLocalize->ConstructString(szBuf, sizeof(szBuf), wszMsg, 1, szValBuf);
	m_pWeaponHelp->InsertString(szBuf);
	m_pWeaponHelp->InsertString("\n");
}

inline void CHudGameplay::InsertTokenWithBar(const char* token, int length)
{
	if (length > HUDWP_BARLENGTH)
		length = HUDWP_BARLENGTH;

	float width = ScreenWidth();
	int height = ScreenHeight();

	float scalevalue = min(sqrt(width / 1680), 1.0);
	
	if ( height % 768 == 0 || height == 720 || height == 800 || height == 1200 || height == 864 ) // Some dinky font glitch makes this neccecery.
		scalevalue *= 0.5;



	int iBarlength = round(HUDWP_BARLENGTH * scalevalue);
	length = round(length * scalevalue);

	m_pWeaponHelp->InsertString(g_pVGuiLocalize->Find(token));
	m_pWeaponHelp->InsertString("\t");
	m_pWeaponHelp->InsertFontChange(m_hWeaponSymbolFont);
	
	for (int i = 0; i < length; i++)
		m_pWeaponHelp->InsertChar('A');

	for (int i = 0; i < iBarlength - length - 1; i++)
		m_pWeaponHelp->InsertChar('B');

	if (iBarlength != length)
		m_pWeaponHelp->InsertChar('C');

	m_pWeaponHelp->InsertString("\n");
	m_pWeaponHelp->InsertFontChange(m_hWeaponFont);
}


//-----------------------------------------------------------------------------
// Purpose: Resizes the label
//-----------------------------------------------------------------------------
void CHudGameplay::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize(wide, tall);

	int infoFontTall = vgui::surface()->GetFontTall(m_hWeaponFont);

	m_pBackground->SetPos(0, 0);
	m_pBackground->SetSize(wide, tall);

	m_pWeaponSet->SetVisible(false);
	m_pWeaponSet->SetPos(0, 0);
	m_pWeaponSet->SetSize(wide - m_iTabWidth, tall);

	m_pGameplayHelp->SetVisible(false);
	m_pGameplayHelp->SetPos(m_iTextX, m_iTextY);
	m_pGameplayHelp->SetSize(wide - m_iTextX - m_iTabWidth, tall - m_iTextY);

	m_pWeaponHelp->SetVisible(false);
	m_pWeaponHelp->SetPos(m_iTextX, m_iTextY);
	m_pWeaponHelp->SetSize(wide - m_iTextX - m_iTabWidth, tall - m_iTextY);

	switch (m_iCurrState)
	{
	case GAMEPLAY_HELP:
		// Set the gameplay help visible
		m_pGameplayHelp->SetVisible(true);
		break;

	case GAMEPLAY_WEAPONSTATS:
		ResolveWeaponHelp();
		m_pWeaponHelp->SetVisible(true);
		break;

	case GAMEPLAY_WEAPONSET:
		m_pWeaponSet->SetVisible(true);
		m_pWeaponSetName->SetPos(m_pWeaponSet->GetWide() / 2 - m_pWeaponSetName->GetWide() / 2, m_iTextY);

		int x_in = m_iTextX, y_in = m_iTextY * 4 + infoFontTall;

		// We list weapons from lowest spawn slot to highest, top to bottom
		for (int i = 0; i < MAX_WEAPON_SPAWN_SLOTS; ++i)
		{
			m_vWeaponLabels[i]->SetPos(x_in, y_in);
			m_vWeaponLabels[i]->pImage->SetPos( x_in + wide - YRES(64) - m_iTabWidth - m_iTextX*2, y_in );
			y_in += m_vWeaponLabels[i]->GetTall() + m_iTextY;
		}
		break;
	}
}

void CHudGameplay::OnThink(void)
{
	// Set our position based on our animation variable
	m_flHeightPercent = clamp(m_flHeightPercent, 0, 1.0f);
	int offset = GetTall() * (1.0f - m_flHeightPercent);
	SetPos(m_iOrigX, m_iOrigY + offset);

	// Hide us and ensure our state is set properly
	if (m_flHeightPercent == 0)
	{
		m_iCurrState = GAMEPLAY_HIDDEN;
		m_iCurrMovement = MOVEMENT_NONE;
		SetVisible(false);
	}
	else if (m_flHeightPercent == 1.0f)
	{
		m_iCurrMovement = MOVEMENT_NONE;
	}
}

void CHudGameplay::PaintString(const wchar_t *text, Color col, vgui::HFont& font, int x, int y)
{
	vgui::surface()->DrawSetTextColor(col);
	vgui::surface()->DrawSetTextFont(font);
	vgui::surface()->DrawSetTextPos(x, y);
	vgui::surface()->DrawUnicodeString(text);
}

void CHudGameplay::KeyboardInput(int action)
{
	if (m_iCurrState == action)
	{
		CGEWeapon *pGEWeapon = NULL;
		CGEPlayer *pGEPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (pGEPlayer)
			pGEWeapon = ToGEWeapon(pGEPlayer->GetActiveWeapon());

		if (m_iCurrState != GAMEPLAY_WEAPONSTATS || !pGEWeapon || pGEWeapon->GetWeaponID() == m_iCurrWepID)
		{
			// Hitting the same key again will hide the panel
			m_pHudAnims->StartAnimationSequence("GameplayHudHide");
			m_iCurrState = GAMEPLAY_HIDDEN;
			m_iCurrMovement = MOVEMENT_HIDING;
			return;
		}
		else
			InvalidateLayout(); // Reload the layout
	}
	else if (m_iCurrState == GAMEPLAY_HIDDEN)
	{
		// If we were hidden, we need to pop up again
		SetVisible(true);
		m_pHudAnims->StartAnimationSequence("GameplayHudShow");
		m_iCurrMovement = MOVEMENT_SHOWING;
	}

	// Set this so we show the correct data
	m_iCurrState = action;
	InvalidateLayout();
}
