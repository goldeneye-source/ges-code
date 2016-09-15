///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_logic_bitflag.h
// Description:
//      Logic entity that will store a bit across multiple rounds.
//		
//
// Created On: 4/28/2016
// Created By: Check Github for list of contributors
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

class CGELogicBitflag : public CLogicalEntity
{
public:
	DECLARE_CLASS(CGELogicBitflag, CLogicalEntity);
	DECLARE_DATADESC();

	CGELogicBitflag();

	// Check the value of the bit and fire the approperate output
	void ReportState(CBaseEntity *pActivator);

	// Set the value of the bit to true
	void InputSetStateTrue(inputdata_t &inputdata);
	// Set the value of the bit to false
	void InputSetStateFalse(inputdata_t &inputdata);
	// Switch the state of the bit
	void InputToggleState(inputdata_t &inputdata);
	// Manually check the bit
	void InputCheckState(inputdata_t &inputdata);
	// Set and then test the bit
	void InputSetFalseCheck(inputdata_t &inputdata);
	// Set and then test the bit
	void InputSetTrueCheck(inputdata_t &inputdata);
	// Set and then test the bit
	void InputToggleCheck(inputdata_t &inputdata);

	// Trigger all the "FireTrue" outputs as defined by the mapper.
	COutputEvent m_FireTrue;
	// Trigger all the "FireFalse" outputs as defined by the mapper.
	COutputEvent m_FireFalse;

	// The current state of the bit
	bool m_bState;

	//private:
};


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(ge_logic_bitflag, CGELogicBitflag);

BEGIN_DATADESC(CGELogicBitflag)

DEFINE_KEYFIELD(m_bState, FIELD_BOOLEAN, "StartingState"),
DEFINE_INPUTFUNC(FIELD_VOID, "SetStateTrue", InputSetStateTrue),
DEFINE_INPUTFUNC(FIELD_VOID, "SetStateFalse", InputSetStateFalse),
DEFINE_INPUTFUNC(FIELD_VOID, "ToggleState", InputToggleState),
DEFINE_INPUTFUNC(FIELD_VOID, "CheckState", InputCheckState),
DEFINE_INPUTFUNC(FIELD_VOID, "SetFalseCheck", InputSetFalseCheck),
DEFINE_INPUTFUNC(FIELD_VOID, "SetTrueCheck", InputSetTrueCheck),
DEFINE_INPUTFUNC(FIELD_VOID, "ToggleCheck", InputToggleCheck),
DEFINE_OUTPUT(m_FireTrue, "OnFireTrue"),
DEFINE_OUTPUT(m_FireFalse, "OnFireFalse"),

END_DATADESC()

CGELogicBitflag::CGELogicBitflag()
{
	m_bState = false;
}

void CGELogicBitflag::ReportState(CBaseEntity *pActivator)
{
	if (!pActivator)
		pActivator = this;

	if (m_bState)
		m_FireTrue.FireOutput(pActivator, this);
	else
		m_FireFalse.FireOutput(pActivator, this);
}

void CGELogicBitflag::InputSetStateTrue(inputdata_t &inputdata)
{
	m_bState = true;
}

void CGELogicBitflag::InputSetStateFalse(inputdata_t &inputdata)
{
	m_bState = false;
}

void CGELogicBitflag::InputToggleState(inputdata_t &inputdata)
{
	m_bState = !m_bState;
}

void CGELogicBitflag::InputCheckState(inputdata_t &inputdata)
{
	ReportState(inputdata.pActivator);
}

void CGELogicBitflag::InputSetFalseCheck(inputdata_t &inputdata)
{
	m_bState = false;
	ReportState(inputdata.pActivator);
}

void CGELogicBitflag::InputSetTrueCheck(inputdata_t &inputdata)
{
	m_bState = true;
	ReportState(inputdata.pActivator);
}

void CGELogicBitflag::InputToggleCheck(inputdata_t &inputdata)
{
	m_bState = !m_bState;
	ReportState(inputdata.pActivator);
}