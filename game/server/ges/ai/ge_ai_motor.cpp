///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai_motor.cpp
//   Description :
//      [TODO: Write the purpose of ge_ai_motor.cpp.]
//
//   Created On: 6/14/2009 10:27:50 AM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_ai_motor.h"
#include "ge_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GE_OLD_AI
ConVar ge_bot_debugyaw("ge_bot_debugyaw", "0");
#endif

void UTIL_AngleMod(QAngle &ang)
{
	ang.x = UTIL_AngleMod( ang.x );
	ang.y = UTIL_AngleMod( ang.y );
	ang.z = UTIL_AngleMod( ang.z );
}



CGEAIMotor::CGEAIMotor(CGEAIHandle* baseAi) : CGEAIHelper(baseAi)
{
	m_fLastThinkTime = gpGlobals->curtime;
	m_fIdealYaw = GetPlayer()->GetAbsAngles().y;
}


CGEAIMotor::~CGEAIMotor()
{

}



void CGEAIMotor::Think()
{
	float deltaTime = gpGlobals->curtime-m_fLastThinkTime;

	if (!IsAtIdealPos())
	{

	}

	if (!IsAtIdealYaw())
	{
		UpdateYaw(-1, deltaTime);
	}

	if (!IsAtIdealAimAngle())
	{
		UpdateAimAng(-1, deltaTime);
	}

	m_fLastThinkTime = gpGlobals->curtime;
}



void CGEAIMotor::SetTargetAimAngle(QAngle ang)
{
	m_aIdealAng = ang;
}


void CGEAIMotor::SetTargetPos(Vector pos)
{
	m_vIdealPos = pos;
}
	

void CGEAIMotor::SetTargetYaw(float yaw)
{
	m_fIdealYaw = yaw;
}
	


bool CGEAIMotor::IsAtIdealAimAngle()
{
	return (GetPlayer()->GetAbsAngles() == m_aIdealAng);
}


bool CGEAIMotor::IsAtIdealPos()
{
	return (GetPlayer()->GetAbsOrigin().DistTo(m_vIdealPos) < 50);
}


bool CGEAIMotor::IsAtIdealYaw()
{
	float current = UTIL_AngleMod( GetPlayer()->GetAbsAngles().y );
	float ideal = UTIL_AngleMod( GetIdealYaw() );

	return (ideal == current);
}


void CGEAIMotor::UpdateYaw( int yawSpeed, float deltaTime )
{
	float ideal, current, newYaw;
	
	if ( yawSpeed == -1 )
		yawSpeed = GetYawSpeed();

	// NOTE: GetIdealYaw() will never exactly be reached because UTIL_AngleMod
	// also truncates the angle to 16 bits of resolution. So lets truncate it here.
	current = UTIL_AngleMod( GetPlayer()->GetLocalAngles().y );
	ideal = UTIL_AngleMod( GetIdealYaw() );

	// FIXME: this needs a proper interval
	float dt = min( 0.2, deltaTime );
	
	newYaw = ClampAng( (float)yawSpeed * 10.0, current, ideal, dt );
		
	if (newYaw != current)
	{
		m_aFrameAng = GetPlayer()->GetLocalAngles();
		m_aFrameAng.y = newYaw;
		//GetPlayer()->SetLocalAngles(m_aFrameAng);
	}

#ifdef GE_OLD_AI
	if (ge_bot_debugyaw.GetBool())
	{
		QAngle ang = GetPlayer()->GetAbsAngles();
		Vector pos = GetPlayer()->GetAbsOrigin();

		Vector temp;
		AngleVectors(ang, &temp);

		Vector end = pos + temp*100;
		DebugDrawLine( pos, end, 0, 255, 255, true, -0.1f );

		ang = QAngle(0,m_fIdealYaw,0);
		AngleVectors(ang, &temp);

		end = pos + temp*100;
		DebugDrawLine( pos, end, 0, 255, 0, true, -0.1f );
	}
#endif
}


void CGEAIMotor::UpdateAimAng( int rotSpeed, float deltaTime )
{
	if ( rotSpeed == -1 )
		rotSpeed = GetAngSpeed();

	QAngle pAng = GetPlayer()->GetAbsAngles();
	QAngle iAng = GetIdealAngle();

	// NOTE: GetIdealYaw() will never exactly be reached because UTIL_AngleMod
	// also truncates the angle to 16 bits of resolution. So lets truncate it here.
	UTIL_AngleMod(pAng);
	UTIL_AngleMod(iAng);

	// FIXME: this needs a proper interval
	float dt = min( 0.2, deltaTime );	

	QAngle angles = ClampRot((float)rotSpeed * 10.0, pAng, iAng, dt);

	m_aFrameAng = angles;
	//GetPlayer()->SnapEyeAngles( angles );
}


QAngle CGEAIMotor::ClampRot( float rotSpeedPerSec, QAngle current, QAngle target, float time )
{
	QAngle res;
	res.x=ClampAng(rotSpeedPerSec, current.x, target.x, time);
	res.y=ClampAng(rotSpeedPerSec, current.y, target.y, time);
	res.z=ClampAng(rotSpeedPerSec, current.z, target.z, time);
	return res;
}


float CGEAIMotor::ClampAng( float rotSpeedPerSec, float current, float target, float time )
{
	if (current != target)
	{
		float speed = rotSpeedPerSec * time;
		float move = target - current;

		if (target > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}

		if (move > 0)
		{// turning to the npc's left
			if (move > speed)
				move = speed;
		}
		else
		{// turning to the npc's right
			if (move < -speed)
				move = -speed;
		}
		
		return UTIL_AngleMod(current + move);
	}
	
	return target;
}



float CGEAIMotor::GetMovSpeed()
{
	return 10;
}


float CGEAIMotor::GetAngSpeed()
{
	return 10;
}


float CGEAIMotor::GetYawSpeed()
{
	return 100;
}


void CGEAIMotor::FillUserCommand(CUserCmd &cmd)
{
	VectorCopy( m_aFrameAng, cmd.viewangles );

	cmd.forwardmove = 0;
	cmd.sidemove = 0;
	cmd.upmove = 0;

	//cmd.buttons = buttons;
	//cmd.impulse = impulse;
}
