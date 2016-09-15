//////////  Copyright © 2008, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_loadout.h
// Description:
//      Stores the loadout data parsed by CGELoadoutParser
//
// Created On: 3/22/2008
// Edited On: 7/8/2011
// Created By: Killermonkey
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_LOADOUT_DATA_H
#define GE_LOADOUT_DATA_H

#define LOADOUT_IDENT_LEN	32
#define LOADOUT_WEAP_LEN	32
#define LOADOUT_NAME_LEN	32

class CGELoadout
{
public:
	DECLARE_CLASS_NOBASE( CGELoadout );

	CGELoadout();
	virtual ~CGELoadout();

	// This should be called before using the weapon set to ensure it is setup properly
	void Activate( void );

	const char* GetIdent( void )		const { return m_szIdent;	}
	const char* GetPrintName( void )	const { return m_szPrintName; }
	bool		CanChooseRandom( void )	const { return m_iWeight > 0; }
	
	// Get the active weapon in the slot provided
	virtual int  GetWeapon( int slot ) const;
	virtual CUtlVector<int>& GetWeaponList() const { return const_cast<CUtlVector<int>&>(m_vActiveWeapons); }

	// Guarantees to return the first valid weapon in the set (skips over WEAPON_NONE's)
	virtual int GetFirstWeapon( void ) const;

	void SetIdent( const char* szIdent )	{ Q_strncpy(m_szIdent, szIdent, LOADOUT_IDENT_LEN); }
	void SetPrintName( const char* szName )	{ Q_strncpy(m_szPrintName, szName, LOADOUT_NAME_LEN); }
	void SetWeight( int weight )			{ m_iWeight = weight; }
	void SetGroup(int type)					{ m_iGroup = type; }
	int  GetWeight()						{ return m_iWeight; }
	int  GetGroup()							{ return m_iGroup; }

	// Used to deconflict with tokens
	void SwitchActiveWeapon( int slot );

	// Handle loading weapons from KeyValues
	void LoadWeapons( KeyValues *pWeapons );

	// Nice debug command
	void PrintList();
	
protected:
	void ResetWeaponList();
	virtual void SortList();

private:
	int  TranslateWeaponName( const char* szName );
	void ChooseRandomWeapons( void );
	int PickGroupWeapon(CUtlVector<int> &weapons, CUtlVector<int> &weights, CUtlVector<int> &strengths, int basegroup, int allowexplosives);

	char m_szIdent[LOADOUT_IDENT_LEN];
	char m_szPrintName[LOADOUT_NAME_LEN];

	int m_iWeight;
	int m_iGroup;
	
	// Weapons for this loadout as GEWeaponID
	// Loaded is what gets used for SetWeapon(..) calls
	// Actual is what gets used for GetWeapon(..) calls and is populated when the loadout is activated
	CUtlVector<int> m_vActiveWeapons;
	CUtlVector<int>	m_vLoadedWeapons;
};

#endif //GE_LOADOUT_DATA_H

