///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_door.h
// Description:
//      Fancy door that accelerates and can change direction during motion.  Also supports paired doors to
//		avoid a lot of the nonsense caused by double door entity systems.
//
// Created On: 9/30/2015
// Created By: Check Github for list of contributors
/////////////////////////////////////////////////////////////////////////////

#include "doors.h"

class CGEDoor : public CBaseDoor
{
public:
	DECLARE_CLASS(CGEDoor, CBaseDoor);
	DECLARE_DATADESC();

	CGEDoor();

	float m_flacceltime; // Time it takes the door to accelerate to max speed
	float m_flAccelSpeed; // Overrides acceltime when used.  Speed that the door accelerates per second.
	float m_flMinSpeed; // Minimum speed the door will travel when decelerating.
	int m_iUseLimit; // How many times the door can be activated mid-motion before denying any further inputs.
	float m_flTriggerThreshold; // Fraction of total movedistance that triggers an output when passed.
	string_t m_sPartner; // Name of the door that opens and closes with this one
	CUtlVector<CGEDoor*> m_pPartnerEnts; // List of pointers to door entities that open and close with this one
	float m_flThinkInterval; // How often the door integrates velocity.

	float m_flDeccelDist; //Distance within the end that the door must start decelerating.  Might not be the same as acceldist.
	float m_flStartMoveTime; // Time that the current movecycle was started.  Used to check if the door can be closed by someone other than the one who opened it.


	virtual void Spawn();
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void SetPartner(string_t newPartner);
	CGEDoor *GetPartner();
	void BuildPartnerList();
	void DoorGoUp();
	void DoorGoDown();
	void DoorGroupGoUp(bool triggerself);
	void DoorGroupGoDown(bool triggerself);
	void Blocked(CBaseEntity *pOther);
	bool CheckUse(CBaseEntity *pActivator);
	int DoorActivate();
	virtual void MoveThink();
	virtual void CalcMovementValues(Vector startpos, Vector endpos);

	bool CheckPartnerPosistions(int pos); // Checks the posistions of all the door's partners against the input.

	virtual CBaseEntity* GetLastActivator()				{ return m_pLastActivator; }
	virtual void SetLastActivator(CBaseEntity* pEnt)	{ m_pLastActivator = pEnt; }

	void InputClose(inputdata_t &inputdata);
	void InputOpen(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);
	void InputForceToggle(inputdata_t &inputdata);

	void InputCheckMovestate(inputdata_t &inputdata);
	void InputCheckPartnersMovestate(inputdata_t &inputdata);

	COutputEvent m_FirstClose;		// Triggered only on the initial close input.
	COutputEvent m_FirstOpen;		// Triggered only on the initial open input.
	COutputEvent m_PartnersClosed;	// Triggered only when the last door in the partner chain finishes closing.
	COutputEvent m_PartnersOpened;	// Triggered only when the last door in the partner chain finishes opening.
	COutputEvent m_PassThreshold;	// Whenever the door passes the threshold one way or the other.
	COutputEvent m_FarThreshold;	// When the door goes past the threshold
	COutputEvent m_NearThreshold;	// When the door comes back
	COutputEvent m_MovestateTrue;	// Fired when movecheck returns true
	COutputEvent m_MovestateFalse;	// Fired when movecheck returns false

	CBaseEntity *m_pLastActivator; //Last person to use the door.  This is to prevent stuff like someone standing on the other side of a door and closing it every time it gets opened effectively shutting anyone else out.  Also used for trap activator detection.

private:
	float m_flTargetDist; //Distance the door must move to reach the goal posistion

	Vector m_vecAccelDir; //Direction the door accelerates

	float m_flLastMoveCalc; //Time that the door last moved

	float m_flTriggerDistance; //Distance the door needs to be within its destination to fire the output.
	bool m_bPassedThreshold; //Did we pass trigger threshold and fire the output?

	int m_iCurrentUses; // How many times the door has been used this movecycle.

	QAngle m_angFinalDest; // For rotating doors:  Angle the door is moving to.
	QAngle m_angAccelDir; // For rotating doors:  Angle the door accelerates on.

	Vector m_vecGroupCenter; // Center of the door and all its partners, used for calculating rotating door move direction.
};