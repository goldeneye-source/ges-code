///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_logic_gate.h
// Description:
//      Logic entity that will function as a specified logic gate.
//		
//
// Created On: 2/18/2016
// Created By: Check Github for list of contributors
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

class CGELogicGate : public CLogicalEntity
{
public:
	DECLARE_CLASS(CGELogicGate, CLogicalEntity);
	DECLARE_DATADESC();

	CGELogicGate();

	virtual void Spawn();

	// Check the inputs against the current logic gate type and return the output that gate would have given.
	virtual bool CheckGateLogic();
	// Fire the approperate entity outputs for the given state.
	virtual void ReportGateLogic(bool state, CBaseEntity *pActivator = NULL);
	// Change one of the inputs and handle the approperate logic.  First input is false for x, true for y.
	virtual void ChangeInput(bool changeY, bool state, CBaseEntity *pActivator = NULL);

	// Set the type of gate
	void InputSetGateType(inputdata_t &inputdata);
	// Manually check the output of the gate
	void InputCheckGateOutput(inputdata_t &inputdata);

	// Set X input to 1
	void InputXTrue(inputdata_t &inputdata);
	// Set Y input to 1
	void InputYTrue(inputdata_t &inputdata);

	// Set X input to 0
	void InputXFalse(inputdata_t &inputdata);
	// Set Y input to 0
	void InputYFalse(inputdata_t &inputdata);

	// Switch X from 1 to 0 or 0 to 1 depending on current value.
	void InputXToggle(inputdata_t &inputdata);
	// Switch Y from 1 to 0 or 0 to 1 depending on current value.
	void InputYToggle(inputdata_t &inputdata);

	// Enable autofire functionality
	void InputEnableAutofire(inputdata_t &inputdata);
	// Disable autofire functionality
	void InputDisableAutofire(inputdata_t &inputdata);

	// Trigger all the "FireTrue" outputs as defined by the mapper.
	COutputEvent m_FireTrue;
	// Trigger all the "FireFalse" outputs as defined by the mapper.
	COutputEvent m_FireFalse;

	// Should this entity fire after every input change?
	bool m_bAutoFire;
	// The type of gate used.  0 is AND, 1 is OR, 2 is NAND, 3 is NOR, 4 is XOR, 5 is XNOR, 6 is NOT(X only)
	int m_iGateType;
	// The current value of the X input.  Also used as the starting value in hammer.
	bool m_bXValue;
	// The current value of the Y input   Also used as the starting value in hammer.
	bool m_bYValue;

//private:
};