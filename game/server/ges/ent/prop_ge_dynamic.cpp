//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Spawn and use functions for GE:S unique editor-placed triggers.
//
//===========================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "saverestore.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "globalstate.h"
#include "filters.h"
#include "vstdlib/random.h"
#include "triggers.h"
#include "saverestoretypes.h"
#include "hierarchy.h"
#include "bspfile.h"
#include "saverestore_utlvector.h"
#include "physics_saverestore.h"
#include "te_effect_dispatch.h"
#include "ammodef.h"
#include "iservervehicle.h"
#include "movevars_shared.h"
#include "physics_prop_ragdoll.h"
#include "props.h"
#include "RagdollBoogie.h"
#include "EntityParticleTrail.h"
#include "in_buttons.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_lead.h"
#include "gameinterface.h"

#ifdef HL2_DLL
#include "hl2_player.h"
#endif

#include "ge_shareddefs.h"
#include "gemp_gamerules.h"
#include "ge_triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Special prop_dynamic that picks a random skin on spawn, tries to avoid picking the same skin as other monitors.
//-----------------------------------------------------------------------------
class CGEPropDynamic : public CDynamicProp
{
public:
	DECLARE_CLASS(CGEPropDynamic, CDynamicProp);
	DECLARE_DATADESC();

	CGEPropDynamic();

	void Spawn(void);

	virtual int OnTakeDamage(const CTakeDamageInfo &info);

	virtual void PickNewSkin( bool broken = false);
	CBaseEntity *GetPartner(); // We use base entity here so that the prop can pass damage to anything.
	void BuildPartnerList();

	virtual bool OverridePropdata() { return true; }
	virtual void Materialize(void);
	virtual void Event_Killed(const CTakeDamageInfo &info);

	void InputPickNewSkin(inputdata_t &inputdata);
	void InputPickNewBrokenSkin(inputdata_t &inputdata);
	void InputPickNewHealthySkin(inputdata_t &inputdata);
	void InputDestroy(inputdata_t &inputdata);
	void InputRespawn(inputdata_t &inputdata);

	int m_iBrokenSkinCount; // Amount of possible skins the model will use when broken.  The fist set of skins in the model.
	int m_bUseRandomSkins; // Use random skins when healthy or not
	int m_iBreakHealth; // Amount of health to use as the reference point for switching skins.

	int m_iHealthOverride; // Hack to override health through hammer for a little more flexibility over the other override method.
	float m_flRespawnTime; // Time to respawn after breaking
	string_t m_sDamageTarget; // Name of the entities that get damaged when this one does
	CUtlVector<CBaseEntity*> m_vDmgTargetList; // List of entities that have that name
	bool m_bRobustSpawn; // If we should use collision checks or not when we respawn
	bool m_bPlCollide; // If we use a collision group that can collide with players or not

	COutputEvent m_Respawn;	// Triggered when the prop respawns

private:
	bool m_bUsingBrokenSkin;
	int m_iStartingSkin;
};


LINK_ENTITY_TO_CLASS(prop_ge_dynamic, CGEPropDynamic);

BEGIN_DATADESC(CGEPropDynamic)

DEFINE_KEYFIELD(m_iBrokenSkinCount, FIELD_INTEGER, "BrokenSkinCount"),
DEFINE_KEYFIELD(m_bUseRandomSkins, FIELD_BOOLEAN, "UseRandomSkins"),
DEFINE_KEYFIELD(m_iBreakHealth, FIELD_INTEGER, "BreakHealth"),
DEFINE_KEYFIELD(m_iHealthOverride, FIELD_INTEGER, "HealthOverride"),
DEFINE_KEYFIELD(m_sDamageTarget, FIELD_STRING, "DamageTransferTarget"),
DEFINE_KEYFIELD(m_flRespawnTime, FIELD_FLOAT, "RespawnTime"),
DEFINE_KEYFIELD(m_bRobustSpawn, FIELD_BOOLEAN, "RobustSpawn"),
DEFINE_KEYFIELD(m_bPlCollide, FIELD_BOOLEAN, "PlCollide"),

DEFINE_INPUTFUNC(FIELD_VOID, "PickRandomSkin", InputPickNewSkin),
DEFINE_INPUTFUNC(FIELD_VOID, "PickRandomBrokenSkin", InputPickNewBrokenSkin),
DEFINE_INPUTFUNC(FIELD_VOID, "PickRandomHealthySkin", InputPickNewHealthySkin),
DEFINE_INPUTFUNC(FIELD_VOID, "Destroy", InputDestroy),
DEFINE_INPUTFUNC(FIELD_VOID, "Respawn", InputRespawn),

DEFINE_OUTPUT(m_Respawn, "OnRespawn"),

END_DATADESC()

CGEPropDynamic::CGEPropDynamic()
{
	// Set these values here incase they were not assigned by the mapper somehow.

	m_iBrokenSkinCount = 0;
	m_bUseRandomSkins = false;
	m_iBreakHealth = 0;
	m_bUsingBrokenSkin = false;
	m_iStartingSkin = 0;
}


void CGEPropDynamic::Spawn(void)
{
	BaseClass::Spawn();

	m_bUsingBrokenSkin = false;
	m_iStartingSkin = m_nSkin;

	if (m_bUseRandomSkins)
		PickNewSkin();

	if (m_iHealthOverride > 0)
	{
		m_takedamage = DAMAGE_YES;
		m_iHealth = m_iMaxHealth = m_iHealthOverride;
	}
	else if (m_iHealthOverride == 0)
	{
		m_takedamage = DAMAGE_NO;
		m_iHealth = m_iMaxHealth = 0;
	}
	else
	{
		m_iHealthOverride = m_iMaxHealth = m_iHealth; //HealthOverride is also our respawn health value, so set it to prop health if it isn't overriding anything.
	}

	if (!m_bPlCollide)
		SetCollisionGroup(COLLISION_GROUP_DEBRIS);

	CBaseEntity *pEntity = gEntList.FindEntityByName(NULL, m_sDamageTarget);

	for (int i = 0; i < 128; i++)
	{
		if (!pEntity)
			break;

		m_vDmgTargetList.AddToTail(pEntity);

		pEntity = gEntList.FindEntityByName(pEntity, m_sDamageTarget);
	}
}

void CGEPropDynamic::Materialize(void)
{
	//A more lightweight and optional func_rebreakable materalize function 
	CreateVPhysics();

	if (m_bRobustSpawn)
	{
		// iterate on all entities in the vicinity.
		CBaseEntity *pEntity;
		Vector max = CollisionProp()->OBBMaxs();
		for (CEntitySphereQuery sphere(GetAbsOrigin(), max.NormalizeInPlace()); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
		{
			if (pEntity == this || pEntity->GetSolid() == SOLID_NONE || pEntity->GetSolid() == SOLID_BSP || pEntity->GetSolidFlags() & FSOLID_NOT_SOLID || pEntity->GetMoveType() & MOVETYPE_NONE)
				continue;

			// Ignore props that can't move
			if (pEntity->VPhysicsGetObject() && !pEntity->VPhysicsGetObject()->IsMoveable())
				continue;

			// Prevent respawn if we are blocked by anything and try again in 1 second
			if (Intersects(pEntity))
			{
				SetSolid(SOLID_NONE);
				AddSolidFlags(FSOLID_NOT_SOLID);
				VPhysicsDestroyObject();
				SetNextThink(gpGlobals->curtime + 1.0f);
				return;
			}
		}
	}

	m_iHealth = m_iHealthOverride;

	if (m_iHealth > 0)
		m_takedamage = DAMAGE_YES;

	RemoveEffects(EF_NODRAW);
	RemoveSolidFlags(FSOLID_NOT_SOLID);

	m_bUsingBrokenSkin = false;

	if (m_bUseRandomSkins)
		PickNewSkin();
	else
		m_nSkin = m_iStartingSkin;

	SetRenderColor(m_col32basecolor.r, m_col32basecolor.g, m_col32basecolor.b);

	m_Respawn.FireOutput(this, this);
}

int CGEPropDynamic::OnTakeDamage(const CTakeDamageInfo &info)
{
	// We don't want to create an infinite chain of damage events, so check to see if the inflictor of this damage is one of our targets and do not reflect it back if it is.
	// It's still possible to create a chain if the chain has 3 members that go in sequence, but I can't see any real use for this kind of setup.  If it becomes an issue this system can be made more robust.
	bool DamageFromTarget = false;
	for (int i = 0; i < m_vDmgTargetList.Count(); i++)
	{
		if (m_vDmgTargetList[i] == info.GetAttacker())
		{
			DamageFromTarget = true;
			break;
		}
	}
	
	if (!DamageFromTarget)
	{
		for (int i = 0; i < m_vDmgTargetList.Count(); i++)
		{
			m_vDmgTargetList[i]->TakeDamage(info);
		}
	}

	int ret = BaseClass::OnTakeDamage(info);

	if ((m_iBreakHealth == 0 || m_iHealth <= m_iBreakHealth) && m_iBrokenSkinCount > 0 && !m_bUsingBrokenSkin)
		PickNewSkin(true);

	return ret;
}


void CGEPropDynamic::Event_Killed(const CTakeDamageInfo &info)
{
	// More or less just the kill event copied straight from prop_physics_respawnable, though it does not teleport as it cannot move on its own.

	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if (pPhysics && !pPhysics->IsMoveable())
	{
		pPhysics->EnableMotion(true);
		VPhysicsTakeDamage(info);
	}

	Break(info.GetInflictor(), info);

	PhysCleanupFrictionSounds(this);

	VPhysicsDestroyObject();

	CBaseEntity::PhysicsRemoveTouchedList(this);
	CBaseEntity::PhysicsRemoveGroundList(this);
	DestroyAllDataObjects();

	AddEffects(EF_NODRAW);

	if (IsOnFire() || IsDissolving())
	{
		UTIL_Remove(GetEffectEntity());
	}

	SetContextThink(NULL, 0, "PROP_CLEARFLAGS");

	if (m_flRespawnTime > 0)
	{
		SetThink(&CGEPropDynamic::Materialize);
		SetNextThink(gpGlobals->curtime + m_flRespawnTime);
	}
}


void CGEPropDynamic::PickNewSkin(bool broken)
{
	// Basically a way of making sure each entity gets a unique skin.
	// We multiply the different coordinates with different coefficients in order to prevent certain arrangements from generating the same seed.
	// aka (-2, 0, 0) is the same as (-1, -1, 0) and many more.  Overlap is obviously still possible with this system too, but there are only a few
	// circumstances where the overlap would occour in locations near eachother.
	RandomSeed((int)(GetAbsOrigin().x * 100 + GetAbsOrigin().y * 10 + GetAbsOrigin().z) + GEMPRules()->GetRandomSeedOffset() + gpGlobals->curtime);

	int healthyskins = GetModelPtr()->numskinfamilies() - m_iBrokenSkinCount;

	if (broken)
	{
		m_nSkin = rand() % m_iBrokenSkinCount;
		m_bUsingBrokenSkin = true;
	}
	else
	{
		m_nSkin = m_iBrokenSkinCount + rand() % healthyskins;
		m_bUsingBrokenSkin = false;
	}
}


void CGEPropDynamic::InputPickNewSkin(inputdata_t &inputdata)
{
	if (m_bUsingBrokenSkin)
		PickNewSkin(true);
	else
		PickNewSkin(false);
}

void CGEPropDynamic::InputPickNewBrokenSkin(inputdata_t &inputdata)
{
	PickNewSkin(true);
}

void CGEPropDynamic::InputPickNewHealthySkin(inputdata_t &inputdata)
{
	PickNewSkin();
}

void CGEPropDynamic::InputDestroy(inputdata_t &inputdata)
{
	CTakeDamageInfo destroyinfo;

	destroyinfo.SetDamage(m_iHealth);
	destroyinfo.SetAttacker(inputdata.pActivator);
	destroyinfo.SetInflictor(this);

	TakeDamage(destroyinfo);
}

void CGEPropDynamic::InputRespawn(inputdata_t &inputdata)
{
	SetThink(&CGEPropDynamic::Materialize);
	SetNextThink(gpGlobals->curtime);
}
