///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai_basenpc.cpp
//   Description :
//      [TODO: Write the purpose of ge_ai_basenpc.cpp.]
//
//   Created On: 9/9/2009 7:47:54 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_ai_basenpc.h"
#include "in_buttons.h"

#include "movehelper_server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CLASS( ge_basempnpc, CGEAIBaseMPNpc );

//BEGIN_DATADESC( CGEAIBaseMPNpc )
//END_DATADESC()

PRECACHE_REGISTER(ge_basempnpc);


void RunPlayerMove( CGEPlayer *fakeclient, CUserCmd &cmd )
{
	if ( !fakeclient )
		return;

	MoveHelperServer()->SetHost( fakeclient );

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	fakeclient->SetTimeBase( gpGlobals->curtime );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );
	fakeclient->SetLastUserCommand( cmd );			// save off the last good usercmd
	fakeclient->pl.fixangle = FIXANGLE_NONE;		// Clear out any fixangle that has been set

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}


CGEAISense* CGEAIBaseNpcProxie::GetSense()
{
	return m_pSense;
}

CGEAIMotor* CGEAIBaseNpcProxie::GetMotor()
{
	return m_pMotor;
}

CGEAINavigator* CGEAIBaseNpcProxie::GetNavigator()
{
	return m_pNavigator;
}

CGEPlayer* CGEAIBaseNpcProxie::GetPlayer()
{
	return m_pPlayer;
}

CGEMPPlayer* CGEAIBaseNpcProxie::GetMPPlayer()
{
	return dynamic_cast<CGEMPPlayer*>(m_pPlayer);
}

CGESPPlayer* CGEAIBaseNpcProxie::GetSPPlayer()
{
	return dynamic_cast<CGESPPlayer*>(m_pPlayer);
}

void CGEAIBaseNpcProxie::PostUserCmd()
{
	CUserCmd cmd;
	Q_memset( &cmd, 0, sizeof( cmd ) );
	cmd.random_seed = random->RandomInt( 0, 0x7fffffff );

	GetMotor()->FillUserCommand(cmd);
	FillUserCommand(cmd);

	GetPlayer()->AddFlag( FL_FAKECLIENT );		// Make sure we stay being a bot
	GetPlayer()->PostClientMessagesSent();		// Fix up the m_fEffects flags
	RunPlayerMove( GetPlayer(), cmd );
}

void CGEAIBaseNpcProxie::Disconnect()
{
	CCommand classcmd;
	classcmd.Tokenize( "disconnect\n" );
	m_pPlayer->ClientCommand( classcmd );
}

void CGEAIBaseNpcProxie::FaceAt(CBaseEntity *ent)
{
	if (!ent)
		return;

	Vector res = ent->GetAbsOrigin() - m_pPlayer->GetAbsOrigin();

	QAngle ang;
	VectorAngles(res, ang);

	m_pMotor->SetTargetYaw(ang.y);
}

void CGEAIBaseNpcProxie::LookAt(CBaseEntity *ent)
{

}

void CGEAIBaseNpcProxie::AimAt(CBaseEntity *ent, Vector offset)
{
	if (!ent)
		return;

	Vector res = ent->GetAbsOrigin() - m_pPlayer->GetAbsOrigin();

	QAngle ang;
	VectorAngles(res, ang);

	m_pMotor->SetTargetAimAngle(ang);
}

void CGEAIBaseNpcProxie::FireShot()
{
	m_fPrimaryFireTime = 0;
}

void CGEAIBaseNpcProxie::FireBurst(float time)
{
	m_fPrimaryFireTime = gpGlobals->curtime + time;
}

void CGEAIBaseNpcProxie::FillUserCommand(CUserCmd &cmd)
{
	//if( bot_crouch.GetInt() )
	//	cmd.buttons |= IN_DUCK;

	if (m_fPrimaryFireTime == 0)
	{
		m_fPrimaryFireTime = -1;
		cmd.buttons |= IN_ATTACK;
	}
	else if (m_fPrimaryFireTime != -1)
	{
		if (m_fPrimaryFireTime < gpGlobals->curtime)
		{
			cmd.buttons |= IN_ATTACK;
		}
		else
		{
			m_fPrimaryFireTime = -1;
		}
	}
}

