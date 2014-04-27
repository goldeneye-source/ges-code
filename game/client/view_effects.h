#ifdef GE_DLL
// This was moved from view_effects.cpp so it can be subclassed...

#ifndef VIEWEFFECTS_H
#define VIEWEFFECTS_H

#include "ivieweffects.h"

//-----------------------------------------------------------------------------
// Purpose: Screen fade variables
//-----------------------------------------------------------------------------
struct screenfade_t
{
	float		Speed;		// How fast to fade (tics / second) (+ fade in, - fade out)
	float		End;		// When the fading hits maximum
	float		Reset;		// When to reset to not fading (for fadeout and hold)
	byte		r, g, b, alpha;	// Fade color
	int			Flags;		// Fading flags

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
// Purpose: Screen shake variables
//-----------------------------------------------------------------------------
struct screenshake_t 
{
	float	endtime;
	float	duration;
	float	amplitude;
	float	frequency;
	float	nextShake;
	Vector	offset;
	float	angle;
	int		command;

	DECLARE_SIMPLE_DATADESC();
};

void CC_Shake_Stop();

class CViewEffects : public IViewEffects
{
public:

	~CViewEffects()
	{
		ClearAllFades();
	}

	virtual void	Init( void );
	virtual void	LevelInit( void );
	virtual void	GetFadeParams( byte *r, byte *g, byte *b, byte *a, bool *blend );
	virtual void	CalcShake( void );
	virtual void	ApplyShake( Vector& origin, QAngle& angles, float factor );

	virtual void	Shake( ScreenShake_t &data );
	virtual void	Fade( ScreenFade_t &data );
	virtual void	ClearPermanentFades( void );
	virtual void	FadeCalculate( void );
	virtual void	ClearAllFades( void );

	// Save / Restore
	virtual void	Save( ISave *pSave );
	virtual void	Restore( IRestore *pRestore, bool fCreatePlayers );

private:

	void ClearAllShakes();
	screenshake_t *FindLongestShake();

	CUtlVector<screenfade_t *>	m_FadeList;

	CUtlVector<screenshake_t *>	m_ShakeList;
	Vector m_vecShakeAppliedOffset;
	float m_flShakeAppliedAngle;

	int							m_FadeColorRGBA[4];
	bool						m_bModulate;

	friend void CC_Shake_Stop();
};

extern IViewEffects *vieweffects;

#endif
#endif // GE_DLL