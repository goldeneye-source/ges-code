///////////// Copyright © 2009 DesuraNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai_basenpc.h
//   Description :
//      [TODO: Write the purpose of ge_ai_basenpc.h.]
//
//   Created On: 9/8/2009 7:24:22 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#ifndef DESURA_GE_AI_BASENPC_H
#define DESURA_GE_AI_BASENPC_H
#ifdef _WIN32
#pragma once
#endif

#include "gemp_player.h"
#include "gesp_player.h"

#include "ge_ai_motor.h"
#include "ge_ai_navigator.h"
#include "ge_ai_sense.h"

template <class U> class CGEAIBaseNpc;

typedef CGEAIBaseNpc<CGEMPPlayer> CGEAIBaseMPNpc;
typedef CGEAIBaseNpc<CGESPPlayer> CGEAIBaseSPNpc;

class CGEAIBaseNpcProxieCallBacks
{
public:
	virtual void Sense()=0;
	virtual void GatherConditions()=0;
	virtual int SelectSchedual()=0;
	virtual int IFF(CGEPlayer* player)=0;
	virtual void PutInServer() = 0;
	virtual const char* GetType()=0;

	virtual void OnKilled()=0;

	virtual int GetSoundPriority(CSound* sound)=0;
	virtual bool ShouldIgnoreSound(CSound* sound)=0;
	virtual int GetSoundInterests()=0;
	virtual float GetHearingSensitivity()=0;
	virtual bool CanHearSound(CSound* sound)=0;
};

//due to the way that new npc's need to be a proper entity, 
//we use a proxie class to be able to extend them via python
class CGEAIBaseNpcProxie : public CGEAIHandle, public CGEAIBaseNpcProxieCallBacks
{
public:
	CGEAIBaseNpcProxie(CGEPlayer* player)
	{
		m_pPlayer = player;
		m_pSense = new CGEAISense(this);
		m_pMotor = new CGEAIMotor(this);
		m_pNavigator = new CGEAINavigator(this);
	}

	~CGEAIBaseNpcProxie()
	{
		delete m_pSense;
		delete m_pMotor;
		delete m_pNavigator;
	}

	CGEAISense* GetSense();
	CGEAIMotor* GetMotor();
	CGEAINavigator* GetNavigator();
	CGEPlayer* GetPlayer();
	CGEMPPlayer* GetMPPlayer();
	CGESPPlayer* GetSPPlayer();

	void PostUserCmd();
	void Disconnect();

	void FaceAt(CBaseEntity *ent);
	void LookAt(CBaseEntity *ent);
	void AimAt(CBaseEntity *ent, Vector offset);

	void FireShot();
	void FireBurst(float time);

protected:
	void FillUserCommand(CUserCmd &cmd);

private:
	float m_fPrimaryFireTime;

	CGEAISense* m_pSense;
	CGEAIMotor* m_pMotor;
	CGEAINavigator* m_pNavigator;
	CGEPlayer* m_pPlayer;
};


template <class U>
class CGEAIBaseNpc : public U
{
public:
	DECLARE_CLASS( CGEAIBaseNpc, U );
	//DECLARE_DATADESC();

	CGEAIBaseNpc()
	{
		m_pProxie = NULL;
	}

	~CGEAIBaseNpc()
	{
		m_pProxie = NULL;
	}

	void SetProxie(CGEAIBaseNpcProxie* proxie)
	{
		m_pProxie = proxie;
	}

	static CGEPlayer *CreateNPCPlayer( edict_t *ed )
	{
		CGEPlayer::s_PlayerEdict = ed;
		return (CGEPlayer*)CreateEntityByName( "ge_basempnpc" );
	}

	void Event_Killed(const CTakeDamageInfo &info)
	{
		U::Event_Killed(info);
		m_pProxie->OnKilled();
	}

private:
	CGEAIBaseNpcProxie* m_pProxie;
};

#endif //DESURA_GE_AI_BASENPC_H
