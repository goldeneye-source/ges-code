///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_door_interp.h
// Description:
//      Func ge_door that moves independantly on client and server to prevent jittery elevators.
//		This entity is absurdly hacked together and only meant to serve one purpose.
//		It doesn't justify the amount of time which would be required for the overhaul neccecery for a sensible fix to elevator
//		jitter.
//
// Created On: 2/20/2016
// Created By: Check Github for list of contributors
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_GEDoorInterp : public C_BaseEntity
{
	DECLARE_CLASS(C_GEDoorInterp, C_BaseEntity);

public:
	DECLARE_CLIENTCLASS();

	C_GEDoorInterp();

	void Spawn();
	void ClientThink();

	EHANDLE m_pTargetDoor;
	Vector m_vecOffsetAverage;
	Vector m_vecTargetPosAverage;
	Vector m_vecSpawnPos;
};

IMPLEMENT_CLIENTCLASS_DT(C_GEDoorInterp, DT_GEDoorInterp, CGEDoorInterp)
	RecvPropEHandle(RECVINFO(m_pTargetDoor)),
	RecvPropVector(RECVINFO(m_vecSpawnPos)),
END_RECV_TABLE()

C_GEDoorInterp::C_GEDoorInterp()
{
	m_vecOffsetAverage = vec3_origin;
	m_pTargetDoor = NULL;
	m_vecSpawnPos = vec3_origin;
}

void C_GEDoorInterp::Spawn(void)
{
	m_pTargetDoor = NULL;
	SetLocalOrigin(m_vecSpawnPos);

	BaseClass::Spawn();

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_GEDoorInterp::ClientThink()
{
	BaseClass::ClientThink();

	// If we don't have something to follow around, don't bother.
	if (!m_pTargetDoor)
		return;

	//This is basically a bunch of nonsense in a desperate attempt to reduce elevator jitter.  All intuitive approaches
	//that I could think of have failed me, this is all that's left.

	//Maybe one day someone else will figure out the core issue and finally do away with this poor compromise.


	//In the interests of anyone who comes after me:

	//Basically, there are two things that make elevators dinky in source.
	//The first is cl_smooth, which is actually not too hard to account for.  It basically just averages the prediction errors
	//concerning the player's posistion over however many frames and then offsets their view accordingly.  It causes the player's
	//view to sink into the elevator though so it's worth trying to account for.
	//I did so by just averaging the posistion of the elevator over the last 1/10th of a second, which is the default value for
	//cl_smooth, but there are more direct ways of accounting for it like:

	//  Vector PosistionOffset;
	//	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	//  pPlayer->GetPredictionErrorSmoothingVector(PosistionOffset);

	// And then add PosistionOffset to the calculated posistion of the elevator.  My attempts at doing this ended up kind of
	// jittery though.


	//The other problem is some sort of nonsense that happens when the player rides an entity.  All other moving entities suddenly
	//get interpolated really poorly and jump around everywhere.  I've tried a ton of stuff to combat this, but it seems
	//to even happen to purely clientside entities.  (I had this clientside entity just constantly set its origin with SetAbsOrigin
	//directly on top of the player's view and it still jittered around when I rode the elevator.)  If you can figure out
	//what actually causes this then the rest should fall into place.  I do know that it became much worse when we changed
	//cl_interp from 0.1 to 0.030303 but as far as i can tell clientside entities aren't affected by that.  Good luck!

	//Anyway here's my shoddy strat.  It just does a rough average of the posistion of the door over the last 1/10th of
	//a second and uses that as the client posistion.  This algorthem doesn't calculate a true average, but it should
	//be close enough for our purposes.


	Vector TargetPos = m_pTargetDoor->GetLocalOrigin();

	float calcTime = gpGlobals->frametime;


	if (calcTime > 0.1 || (m_vecTargetPosAverage - TargetPos).LengthSqr() < 1.0) //If our calctime is too large or we're super close, just set them equal.
		m_vecTargetPosAverage = TargetPos;
	else
	{
		if (m_vecTargetPosAverage == vec3_origin)
			m_vecTargetPosAverage = TargetPos;
		else
		{
			m_vecTargetPosAverage *= 1.00 - calcTime * 10;
			m_vecTargetPosAverage += TargetPos * calcTime * 10;
		}
	}

	SetLocalOrigin(m_vecTargetPosAverage);

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}
