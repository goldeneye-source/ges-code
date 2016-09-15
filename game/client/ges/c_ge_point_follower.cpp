///////////// Copyright © 2013, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_capturearea.cpp
// Description:
//      Implements glow for capture areas on clients
//
// Created On: 28 Mar 2013
// Created By: Jonathan White <killermonkey> 
////////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_GEFollower : public C_BaseEntity
{
	DECLARE_CLASS(C_GEFollower, C_BaseEntity);

public:
	DECLARE_CLIENTCLASS();

	C_GEFollower();

	void Spawn();
	void ClientThink();

	float m_flInterpInterval;

	EHANDLE m_pTargetEnt;

private:
	Vector m_vecSimPos; // Independantly tracked posistion because it randomly resets otherwise.
	Vector m_vecPrevVelocity; // Previously used velocity, used to prevent the follower from randomly jumping around.

	float m_flPrevSpeed; // Previously used speed, used to prevent the follower from randomly jumping around.
	bool m_bForcePos;

	float m_flNextSampleTime;
	Vector m_vecInterpPosistions[10];
};

IMPLEMENT_CLIENTCLASS_DT(C_GEFollower, DT_GEFollower, CGEFollower)
	RecvPropFloat(RECVINFO(m_flInterpInterval)),
	RecvPropEHandle(RECVINFO(m_pTargetEnt)),
END_RECV_TABLE()

C_GEFollower::C_GEFollower()
{
	m_flInterpInterval = 0.1;
	m_vecSimPos = vec3_origin;

	m_bForcePos = true;
	m_pTargetEnt = NULL;
}

void C_GEFollower::Spawn(void)
{
	BaseClass::Spawn();

	m_flNextSampleTime = 0;
	m_bForcePos = true;

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_GEFollower::ClientThink()
{
	if (m_pTargetEnt && IsAbsQueriesValid())
	{
		Vector TargetPos = m_pTargetEnt->GetLocalOrigin();

		if (m_bForcePos)
		{
			m_bForcePos = false;
			m_vecSimPos = TargetPos;

			for (int i = 0; i < 10; i++)
			{
				m_vecInterpPosistions[i] = m_vecSimPos; //Set all the posistions to the target entity's current posistion.
			}
		}

		if (m_flNextSampleTime < gpGlobals->curtime)
		{
			for (int i = 0; i < 9; i++)
			{
				m_vecInterpPosistions[i] = m_vecInterpPosistions[i + 1]; //Shift all of the interp posistions over one.
			}

			m_vecInterpPosistions[9] = TargetPos; //Add the current posistion of the target to the very end of the list.
			m_flNextSampleTime = gpGlobals->curtime + m_flInterpInterval * 0.1;
		}

		Vector CurVelocity = (m_vecInterpPosistions[0] - m_vecSimPos) / (m_flInterpInterval * 0.1);

		bool isConverging = true;

		// Check every interp posistion except the final one against the target pos.  If they all match the follower is about to stop.
		for (int i = 1; i < 10; i++)
		{
			if (m_vecInterpPosistions[i] != TargetPos)
				isConverging = false;
		}

		if (isConverging && TargetPos == m_vecSimPos)
		{
			m_flPrevSpeed = 0;
			SetLocalOrigin(m_vecSimPos);
		}
		else if (isConverging && (TargetPos - m_vecSimPos).LengthSqr() < (CurVelocity*gpGlobals->frametime).LengthSqr()) //Make sure we're not about to move past our target p
		{
			m_flPrevSpeed = 0;
			m_vecSimPos = TargetPos;
			SetLocalOrigin(m_vecSimPos);
		}
		else
		{
			m_vecSimPos = m_vecSimPos + CurVelocity*gpGlobals->frametime;
			SetLocalOrigin(m_vecSimPos);
		}
	}

	BaseClass::ClientThink();

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}