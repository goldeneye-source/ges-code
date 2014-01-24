#ifndef GE_AI_HELPER_H
#define GE_AI_HELPER_H

class CGEPlayer;
class CGEAIMotor;
class CGEAISense;
class CGEAINavigator;


//this is here to hide the template ai class from the ai helpers
class CGEAIHandle
{
public:
	virtual CGEPlayer* GetPlayer() = 0;
	virtual CGEAIMotor *GetMotor() = 0;
	virtual CGEAISense *GetSense() = 0;
	virtual CGEAINavigator* GetNavigator() = 0;

	virtual int GetSoundPriority(CSound* sound)=0;
	virtual bool ShouldIgnoreSound(CSound* sound)=0;
	virtual int GetSoundInterests()=0;
	virtual float GetHearingSensitivity()=0;
	virtual bool CanHearSound(CSound* sound)=0;
};

class CGEAIHelper
{
public:
	CGEAIHelper( CGEAIHandle* baseAi ) 
	{ 
		m_pBaseAI = baseAi; 
	};



	CGEPlayer *GetPlayer( void ) 
	{ 
		return m_pBaseAI->GetPlayer(); 
	};

	CGEAIMotor *GetMotor( void ) 
	{ 
		return m_pBaseAI->GetMotor(); 
	};

	CGEAISense* GetSense()
	{
		return m_pBaseAI->GetSense();
	}

	CGEAINavigator* GetNavigator()
	{
		return m_pBaseAI->GetNavigator();
	}

	CGEAIHandle* GetAIHandle()
	{
		return m_pBaseAI;
	}

private:
	CGEAIHandle* m_pBaseAI;
};

#endif