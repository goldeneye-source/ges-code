///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_logic_gate.cpp
// Description:
//      Logic entity that emulates logic gate.
//		
//
// Created On: 2/18/2016
// Created By: Check Github for list of contributors
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "ge_logic_gate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(ge_logic_gate, CGELogicGate);

BEGIN_DATADESC(CGELogicGate)

DEFINE_KEYFIELD(m_bAutoFire, FIELD_BOOLEAN, "ShouldAutoFire"),
DEFINE_KEYFIELD(m_iGateType, FIELD_INTEGER, "GateType"),
DEFINE_KEYFIELD(m_bXValue, FIELD_BOOLEAN, "StartingXValue"),
DEFINE_KEYFIELD(m_bYValue, FIELD_BOOLEAN, "StartingYValue"),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetGateType", InputSetGateType),
DEFINE_INPUTFUNC(FIELD_VOID, "TestOutput", InputCheckGateOutput),
DEFINE_INPUTFUNC(FIELD_VOID, "SetXTrue", InputXTrue),
DEFINE_INPUTFUNC(FIELD_VOID, "SetXFalse", InputXFalse),
DEFINE_INPUTFUNC(FIELD_VOID, "SetYTrue", InputYTrue),
DEFINE_INPUTFUNC(FIELD_VOID, "SetYFalse", InputYFalse),
DEFINE_INPUTFUNC(FIELD_VOID, "ToggleX", InputXToggle),
DEFINE_INPUTFUNC(FIELD_VOID, "ToggleY", InputYToggle),
DEFINE_INPUTFUNC(FIELD_VOID, "EnableAutofire", InputEnableAutofire),
DEFINE_INPUTFUNC(FIELD_VOID, "DisableAutofire", InputDisableAutofire),
DEFINE_OUTPUT(m_FireTrue, "OnFireTrue"),
DEFINE_OUTPUT(m_FireFalse, "OnFireFalse"),
END_DATADESC()


CGELogicGate::CGELogicGate()
{
	m_bAutoFire = true;
	m_iGateType = 0;

	m_bXValue = 0;
	m_bYValue = 0;
}

void CGELogicGate::Spawn(void)
{
	// Fire our gate logic right on spawning so a gate that already returns true reports it.
	if (m_bAutoFire)
		ReportGateLogic(CheckGateLogic());

	BaseClass::Spawn();
}

bool CGELogicGate::CheckGateLogic(void)
{
	bool outputValue = false;
	// We don't need breaks here because all cases return something.
	switch (m_iGateType)
	{
	case 0: // AND
		outputValue = m_bXValue & m_bYValue;
		break;
	case 1: // OR
		outputValue = m_bXValue | m_bYValue;
		break;
	case 2: // NAND
		outputValue = !(m_bXValue & m_bYValue);
		break;
	case 3: // NOR
		outputValue = !(m_bXValue | m_bYValue);
		break;
	case 4: // XOR
		outputValue = m_bXValue ^ m_bYValue;
		break;
	case 5: // XNOR
		outputValue = !(m_bXValue ^ m_bYValue);
		break;
	case 6: // NOT(X input only)
		outputValue = !m_bXValue;
		break;
	default:
		outputValue = false;
	}

	return outputValue;
}

void CGELogicGate::ReportGateLogic(bool state, CBaseEntity *pActivator)
{
	CBaseEntity *pNewActivator = pActivator;

	if (!pNewActivator)
		pNewActivator = this;

	if (state)
		m_FireTrue.FireOutput(pNewActivator, this);
	else
		m_FireFalse.FireOutput(pNewActivator, this);
}

void CGELogicGate::InputSetGateType(inputdata_t &inputdata)
{
	m_iGateType = inputdata.value.Int();

	// If the type of gate changes the output may have changed.
	if (m_bAutoFire)
		ReportGateLogic(CheckGateLogic(), inputdata.pActivator);
}

void CGELogicGate::InputCheckGateOutput(inputdata_t &inputdata)
{
	ReportGateLogic(CheckGateLogic(), inputdata.pActivator);
}

void CGELogicGate::ChangeInput(bool changeY, bool state, CBaseEntity *pActivator)
{
	if (changeY)
		m_bYValue = state;
	else
		m_bXValue = state;

	// Changing an input could have changed the output.
	if (m_bAutoFire)
		ReportGateLogic(CheckGateLogic(), pActivator);
}

void CGELogicGate::InputXTrue(inputdata_t &inputdata)
{
	ChangeInput(false, true, inputdata.pActivator);
}

void CGELogicGate::InputXFalse(inputdata_t &inputdata)
{
	ChangeInput(false, false, inputdata.pActivator);
}

void CGELogicGate::InputYTrue(inputdata_t &inputdata)
{
	ChangeInput(true, true, inputdata.pActivator);
}

void CGELogicGate::InputYFalse(inputdata_t &inputdata)
{
	ChangeInput(true, false, inputdata.pActivator);
}

void CGELogicGate::InputXToggle(inputdata_t &inputdata)
{
	ChangeInput(false, !m_bXValue, inputdata.pActivator);
}

void CGELogicGate::InputYToggle(inputdata_t &inputdata)
{
	ChangeInput(true, !m_bYValue, inputdata.pActivator);
}

void CGELogicGate::InputEnableAutofire(inputdata_t &inputdata)
{
	m_bAutoFire = true;
}

void CGELogicGate::InputDisableAutofire(inputdata_t &inputdata)
{
	m_bAutoFire = false;
}
