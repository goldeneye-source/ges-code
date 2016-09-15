#include "triggers.h"
#include "ge_shareddefs.h"

//-----------------------------------------------------------------------------
// Purpose: Hurts anything that touches it. If it kills someone it will fire off custom kill messages and assign points to the activator.
//-----------------------------------------------------------------------------
class CTriggerTrap : public CTriggerHurt
{
public:
	DECLARE_CLASS(CTriggerTrap, CTriggerHurt);
	DECLARE_DATADESC();

	virtual CBaseEntity* GetTrapOwner()					{ return m_hTrapOwner; }
	virtual void SetTrapOwner(CBaseEntity* pEnt)		{ m_hTrapOwner = pEnt; }

	virtual string_t GetTrapDeathString()				{ return m_sDeathMessage; }
	virtual string_t GetTrapSuicideString()				{ return m_sSuicideMessage; }
	virtual string_t GetTrapKillString()				{ return m_sKillMessage; }

	void Spawn(void);
	// Input handlers
	virtual void InputEnable(inputdata_t &inputdata);
	virtual void InputDisable(inputdata_t &inputdata);
	virtual void InputToggle(inputdata_t &inputdata);
	virtual void InputBecomeOwner(inputdata_t &inputdata);
	virtual void InputVoidOwner(inputdata_t &inputdata);

	CBaseEntity* m_hTrapOwner;	// Who gets credit for what this trap kills?
	string_t	m_sDeathMessage;
	string_t	m_sSuicideMessage;
	string_t	m_sKillMessage;

};