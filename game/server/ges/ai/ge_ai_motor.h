///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai_motor.h
//   Description :
//      [TODO: Write the purpose of ge_ai_motor.h.]
//
//   Created On: 6/14/2009 10:27:45 AM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GE_AI_MOTOR_H
#define MC_GE_AI_MOTOR_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_ai_helper.h"

class CGEAIMotor : public CGEAIHelper
{
public:
	CGEAIMotor(CGEAIHandle* baseAi);
	~CGEAIMotor();

	void FillUserCommand(CUserCmd &cmd);

	void Think();

	void SetTargetAimAngle(QAngle ang);
	void SetTargetPos(Vector pos);
	void SetTargetYaw(float ang);
	
	bool IsAtIdealAimAngle();
	bool IsAtIdealPos();
	bool IsAtIdealYaw();

	QAngle GetIdealAngle(){return m_aIdealAng;}
	Vector GetIdealPos(){return m_vIdealPos;}
	float GetIdealYaw(){return m_fIdealYaw;}

	QAngle GetFrameAngle(){return m_aFrameAng;}

protected:
	float GetMovSpeed();
	float GetAngSpeed();
	float GetYawSpeed();

	void UpdateYaw( int yawSpeed, float deltaTime );
	void UpdateAimAng( int rotSpeed, float deltaTime );

	QAngle ClampRot( float rotSpeedPerSec, QAngle current, QAngle target, float time );
	float ClampAng( float rotSpeedPerSec, float current, float target, float time );

private:
	QAngle m_aFrameAng;
	Vector m_vIdealPos;
	QAngle m_aIdealAng;
	float m_fIdealYaw;

	float m_fLastThinkTime;
};



#endif //MC_GE_AI_MOTOR_H
