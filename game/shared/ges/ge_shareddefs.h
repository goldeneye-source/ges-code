/////////////  Copyright © 2006, Scott Loyd. All rights reserved.  /////////////
// 
// ge_shareddefs.h
//
// Description:
//      Goldeneye specific hardcoded values that we use throughout the code.
//
// Created On: 8/21/2006 1:39:46 PM
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_SHAREDDEFS_H
#define GE_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "wchar.h"

#ifdef GAME_DLL
  #include "valve_minmax_off.h"
  #include <random>
  #include "valve_minmax_on.h"
#endif

#define M_TWOPI		6.2831853071795864f
#define M_HALFPI	1.5707963267948966f

#define MAX_ENTITY_NAME 32
#define MAX_MODEL_PATH	512

#define GES_AUTH_URL		"http://update.geshl2.com/gesauth.txt"
#define GES_VERSION_URL		"http://update.geshl2.com/gesupdate.txt"

// Deal with testing
#ifdef GES_TESTING
extern bool IsTesting();
#endif

//--------------------------------------------------------------
// Take care of some incompatible sharing
//
#ifdef CLIENT_DLL
#define CGEGameplayEventListener C_GEGameplayEventListener
class C_GEGameplayEventListener { };
#endif

//--------------------------------------------------------------------------------------------------------
//
// Ammo Specific Settings - They are used in ge_gamerules.cpp and the weapon scripts.
//
#define AMMO_NONE			""
#define AMMO_9MM			"9mm"
#define AMMO_RIFLE			"rifle"
#define AMMO_BUCKSHOT		"buckshot"
#define AMMO_MAGNUM			"magnum"
#define AMMO_GOLDENGUN		"goldenammo"
#define AMMO_PROXIMITYMINE	"proximitymine"
#define AMMO_TIMEDMINE		"timedmine"
#define AMMO_REMOTEMINE		"remotemine"
#define AMMO_TKNIFE			"tknife"
#define AMMO_MOONRAKER		"moonraker"
#define AMMO_GRENADE		"grenades"
#define AMMO_ROCKET			"rockets"
#define AMMO_SHELL			"shells"

// HUD Icons as defined in mod_textures.txt
#define AMMO_9MM_ICON			"9mm_pickup"
#define AMMO_RIFLE_ICON			"rifle_pickup"
#define AMMO_BUCKSHOT_ICON		"buckshot_pickup"
#define AMMO_MAGNUM_ICON		"magnum_pickup"
#define AMMO_GOLDENGUN_ICON		"goldengun_pickup"
#define AMMO_PROXIMITYMINE_ICON	"proxy_pickup"
#define AMMO_REMOTEMINE_ICON	"remote_pickup"
#define AMMO_TIMEDMINE_ICON		"timed_pickup"
#define AMMO_TKNIFE_ICON		"tknife_pickup"
#define AMMO_GRENADE_ICON		"grenade_pickup"
#define AMMO_SHELL_ICON			"launcher_pickup"
#define AMMO_ROCKET_ICON		"rocket_pickup"
#define AMMO_MOONRAKER_ICON		"moonraker_pickup"

//Max Ammo Definitions
const int AMMO_9MM_MAX			=	800;
const int AMMO_RIFLE_MAX		=	400;
const int AMMO_BUCKSHOT_MAX		=	100;
const int AMMO_MAGNUM_MAX		=	200;
const int AMMO_GOLDENGUN_MAX	=	100;
const int AMMO_PROXIMITYMINE_MAX =	10;
const int AMMO_TIMEDMINE_MAX	=	10;
const int AMMO_REMOTEMINE_MAX	=	10;
const int AMMO_TKNIFE_MAX		=	10;
const int AMMO_GRENADE_MAX		=	12;
const int AMMO_ROCKET_MAX		=	6;
const int AMMO_SHELL_MAX		=	12;
const int AMMO_MOONRAKER_MAX	=	1;

const int AMMO_9MM_CRATE			=	50;
const int AMMO_RIFLE_CRATE			=	75;
const int AMMO_BUCKSHOT_CRATE		=	20;
const int AMMO_MAGNUM_CRATE			=	25;
const int AMMO_GOLDENGUN_CRATE		=	10;
const int AMMO_PROXIMITYMINE_CRATE	=	3;
const int AMMO_TIMEDMINE_CRATE		=	3;
const int AMMO_REMOTEMINE_CRATE		=	3;
const int AMMO_TKNIFE_CRATE			=	5;
const int AMMO_GRENADE_CRATE		=	3;
const int AMMO_ROCKET_CRATE			=	2;
const int AMMO_SHELL_CRATE			=	4;
const int AMMO_MOONRAKER_CRATE		=	0;

//--------------------------------------------------------------------------------------------------------
// Weapon IDs for all GoldenEye Game weapons
//
typedef enum GES_WEAPONS
{
	WEAPON_NONE = 0,

	WEAPON_PP7,
	WEAPON_PP7_SILENCED,
	WEAPON_DD44,
	WEAPON_SILVERPP7,
	WEAPON_GOLDENPP7,
	WEAPON_COUGAR_MAGNUM,
	WEAPON_GOLDENGUN,
	
	WEAPON_SHOTGUN,
	WEAPON_AUTO_SHOTGUN,

	WEAPON_KF7,
	WEAPON_KLOBB,
	WEAPON_ZMG,
	WEAPON_D5K,
	WEAPON_D5K_SILENCED,
	WEAPON_RCP90,
	WEAPON_AR33,
	WEAPON_PHANTOM,
	WEAPON_SNIPER_RIFLE,

	WEAPON_KNIFE_THROWING,

	WEAPON_GRENADE_LAUNCHER,
	WEAPON_ROCKET_LAUNCHER,

	WEAPON_MOONRAKER,
	WEAPON_KNIFE,

	WEAPON_SPAWNMAX, // Don't spawn weapons in GEWeaponSpawner below this!

	// This is so that the mines/grenades spawn in ammo crates only!
	WEAPON_TIMEDMINE,
	WEAPON_REMOTEMINE,
	WEAPON_PROXIMITYMINE,
	WEAPON_GRENADE,

	// This defines the maxium that a random weapon can be defined as
	WEAPON_RANDOM_MAX,

	WEAPON_SLAPPERS,

	WEAPON_RANDOM,		// For weapon spawning system, weighted random weapon.
	WEAPON_EXPLOSION,	// Generic explosion (not from any particular weapon)
	WEAPON_TRAP,		// Map entity kills
	WEAPON_TOKEN,
	WEAPON_RANDOM_HITSCAN,		// For weapon spawning system, weighted random hitscan weapon.
	WEAPON_RANDOM_EXPLOSIVE,	// For weapon spawning system, weighted random explosive weapon.
	WEAPON_ULTRA_RANDOM,		// For weapon spawning system, unweighted random weapon.

	WEAPON_MAX,
} GEWeaponID;

typedef struct
{
	GEWeaponID		id;
	const char		*alias;
	const char		*ammo;
	const char		*printname;
	int				randweight;
	int				strength;
} GEWeaponInfo_t;
	

//--------------------------------------------------------------------------------------------------------
// An array the captures all the weapons in the mod and associates them with there
// spawner name and the type of ammo they have. This is mainly for the spawner system
// to recognize what type of ammo to spawn for each weapon
//
static const GEWeaponInfo_t GEWeaponInfo[] = 
{
	{WEAPON_NONE,			"weapon_none",			AMMO_NONE,		"#GE_NOWEAPON",		0,		-1	},
	
	// These are spawnable weapons (eg they can be created)
	{WEAPON_PP7,			"weapon_pp7",			AMMO_9MM,		"#GE_PP7",			5,		2	},
	{WEAPON_PP7_SILENCED,	"weapon_pp7_silenced",	AMMO_9MM,		"#GE_PP7_SILENCED",	5,		2	},
	{WEAPON_DD44,			"weapon_dd44",			AMMO_9MM,		"#GE_DD44",			10,		2	},
	{WEAPON_SILVERPP7,		"weapon_silver_pp7",	AMMO_MAGNUM,	"#GE_SilverPP7",    10,		7   },
	{WEAPON_GOLDENPP7,		"weapon_golden_pp7",	AMMO_GOLDENGUN, "#GE_GoldPP7",		8,		8   },
	{WEAPON_COUGAR_MAGNUM,	"weapon_cmag",			AMMO_MAGNUM,	"#GE_CougarMagnum",	12,		5	},
	{WEAPON_GOLDENGUN,		"weapon_golden_gun",	AMMO_GOLDENGUN,	"#GE_GoldenGun",	10,		8	},

	{WEAPON_SHOTGUN,		"weapon_shotgun",		AMMO_BUCKSHOT,	"#GE_Shotgun",		12,		5	},
	{WEAPON_AUTO_SHOTGUN,	"weapon_auto_shotgun",	AMMO_BUCKSHOT,	"#GE_AutoShotgun",	10,		7	},

	{WEAPON_KF7,			"weapon_kf7",			AMMO_RIFLE,		"#GE_KF7Soviet",	10,		3	},
	{WEAPON_KLOBB,			"weapon_klobb",			AMMO_9MM,		"#GE_Klobb",		10,		1	},
	{WEAPON_ZMG,			"weapon_zmg",			AMMO_9MM,		"#GE_ZMG",			10,		4	},
	{WEAPON_D5K,			"weapon_d5k",			AMMO_9MM,		"#GE_D5K",			5,		3	},
	{WEAPON_D5K_SILENCED,	"weapon_d5k_silenced",	AMMO_9MM,		"#GE_D5K_SILENCED",	5,		3	},
	{WEAPON_RCP90,			"weapon_rcp90",			AMMO_9MM,		"#GE_RCP90",		10,		6	},
	{WEAPON_AR33,			"weapon_ar33",			AMMO_RIFLE,		"#GE_AR33",			10,		6	},
	{WEAPON_PHANTOM,		"weapon_phantom",		AMMO_9MM,		"#GE_Phantom",		9,		5	},
	{WEAPON_SNIPER_RIFLE,	"weapon_sniper_rifle",	AMMO_RIFLE,		"#GE_SniperRifle",	10,		4	},

	{WEAPON_KNIFE_THROWING, "weapon_knife_throwing",AMMO_TKNIFE,	"#GE_ThrowingKnife",12,		4	},

	{WEAPON_GRENADE_LAUNCHER, "weapon_grenade_launcher",	AMMO_SHELL,	"#GE_GrenadeLauncher",7,		8	},
	{WEAPON_ROCKET_LAUNCHER,  "weapon_rocket_launcher",		AMMO_ROCKET,"#GE_RocketLauncher", 7,		7	},
	
	{WEAPON_MOONRAKER,	"weapon_moonraker",			AMMO_MOONRAKER,	"#GE_Moonraker",  10,		7	},
	{WEAPON_KNIFE,		"weapon_knife",				AMMO_NONE,		"#GE_Knife",	  4,		3  },

	{WEAPON_SPAWNMAX,	NULL,						AMMO_NONE,			"",				  0,		-1	},

	// These spawn in ammo crates only
	{WEAPON_TIMEDMINE,	"weapon_timedmine",			AMMO_TIMEDMINE,		"#GE_TimedMine",  6,		3	},
	{WEAPON_REMOTEMINE, "weapon_remotemine",		AMMO_REMOTEMINE,	"#GE_RemoteMine", 10,		6	},
	{WEAPON_PROXIMITYMINE, "weapon_proximitymine",	AMMO_PROXIMITYMINE,	"#GE_ProximityMine", 1,		5	},
	{WEAPON_GRENADE,	"weapon_grenade",			AMMO_GRENADE,		"#GE_Grenade",	  10,		4 },

	{WEAPON_RANDOM_MAX,	NULL,						AMMO_NONE,			"",				0,		-1	},

	// These are weapons that shouldn't spawn in the weapon spawners!
	{WEAPON_SLAPPERS,	"weapon_slappers",			AMMO_NONE,		"#GE_Slapper",		0,		-1	},

	// For weapon spawning system only!
	{WEAPON_RANDOM,		"weapon_random",			AMMO_NONE,		"",					0,		-1	},
	{WEAPON_EXPLOSION,  NULL,						AMMO_NONE,		"#GE_Explosion",	0,		-1	},
	{WEAPON_TRAP,		NULL,						AMMO_NONE,		"#GE_Trap",			0,		-1  },
	{WEAPON_TOKEN,		NULL,						AMMO_NONE,		"",					0,		-1	},
	{WEAPON_RANDOM_HITSCAN, "weapon_random_hitscan",		AMMO_NONE,		"",				0,		-1	},
	{WEAPON_RANDOM_EXPLOSIVE, "weapon_random_explosive",	AMMO_NONE,		"",				0,		-1	},
	{WEAPON_ULTRA_RANDOM,	"weapon_ultra_random",		AMMO_NONE,		"",					0,		-1	},

	{WEAPON_MAX,		NULL,						AMMO_NONE,		"",					0,		-1		},
};

enum WeaponEvents
{
	WEAPON_EVENT_RELOAD = 0,
	WEAPON_EVENT_DRAW,
	WEAPON_EVENT_HOLSTER,
};

#define AMMOCRATE_SKIN_DEFAULT		0
#define AMMOCRATE_SKIN_GRENADES		1
#define AMMOCRATE_SKIN_MINES		2

#define AMMOCRATE_MODEL	"models/weapons/ammocrate.mdl"
#define BABYCRATE_MODEL	"models/weapons/ammocrate_small.mdl"

#define MAX_WEAPON_SPAWN_SLOTS	8
const int MAX_NET_RADAR_ENTS = MAX_PLAYERS + 10;
enum RadarEntState
{
	RADAR_STATE_NONE = 0,
	RADAR_STATE_DRAW,
	RADAR_STATE_NODRAW,
};

enum RadarType
{
	RADAR_TYPE_DEFAULT = 0,	// Anything that is not special
	RADAR_TYPE_PLAYER,		// A player
	RADAR_TYPE_TOKEN,		// A gameplay token
	RADAR_TYPE_OBJECTIVE,	// A special objective (capture point, etc.)
};

#define RADAR_THINK_INT				0.1f	// Seconds
#define RADAR_MIN_CAMP_TIME			30.0f	// Seconds
#define RADAR_MAX_CAMP_TIME			50.0f	// Seconds
#define RADAR_BAD_CAMP_TIME			75.0f	// Seconds
#define RADAR_CAMP_ALLVIS_PERCENT	75		// Percent

// Functions defined in ge_weapon.cpp to get weapon id to entity name
extern const char *WeaponIDToAlias( int id );
extern int AliasToWeaponID( const char *alias );
extern const char *GetAmmoForWeapon( int id );
extern const char *GetWeaponPrintName( int id );
extern int GetRandWeightForWeapon( int id );
extern int GetStrengthOfWeapon(int id);
extern int WeaponMaxDamageFromID(int id);

// Team colors for the GE Teams
static Color COLOR_JANUS	( 238,  54,  54, 255 );
static Color COLOR_MI6		( 161, 207, 243, 255 );
static Color COLOR_NOTEAM	( 153, 255, 140, 255 );

// Team values
#define TEAM_MI6		2
#define TEAM_JANUS		3
#define MAX_GE_TEAMS	4
// Extra meta-teams for python use
#define TEAM_OBS			5
#define TEAM_NO_OBS			6
#define TEAM_MI6_OBS		7
#define TEAM_MI6_NO_OBS		8
#define TEAM_JANUS_OBS		9
#define TEAM_JANUS_NO_OBS	10

// Scoreboard colors for use with python
enum GEScoreBoardColor
{
	SB_COLOR_NORMAL = 0,
	SB_COLOR_GOLD,
	SB_COLOR_WHITE,
	SB_COLOR_ELIMINATED,
};

//Max amount of armor and health a player has.
const int STARTING_HEALTH = 160;
const int MAX_HEALTH = 160;

const float STARTING_ARMOR = 0;
const float MAX_ARMOR = 160;

#define GE_NORM_SPEED	190
#define GE_AIM_SPEED_MULT	0.58
#define GE_AIMMODE_DELAY	0.1f

#ifdef GAME_DLL
	typedef enum
	{
		SPAWN_NONE = 0,

		SPAWN_FIRST_GESPAWNER = 1,
			SPAWN_AMMO = SPAWN_FIRST_GESPAWNER,
			SPAWN_WEAPON,
			SPAWN_TOKEN,
			SPAWN_TOKEN_MI6,
			SPAWN_TOKEN_JANUS,
		SPAWN_LAST_GESPAWNER = SPAWN_TOKEN_JANUS,

		SPAWN_FIRST_POINTENT = SPAWN_LAST_GESPAWNER + 1,
			SPAWN_PLAYER = SPAWN_FIRST_POINTENT,
			SPAWN_PLAYER_MI6,
			SPAWN_PLAYER_JANUS,
			SPAWN_PLAYER_SPECTATOR,
			SPAWN_CAPAREA,
			SPAWN_CAPAREA_MI6,
			SPAWN_CAPAREA_JANUS,
		SPAWN_LAST_POINTENT = SPAWN_CAPAREA_JANUS,

		SPAWN_MAX,
	} GESpawnerType;

	typedef struct
	{
		GESpawnerType	id;
		const char	*classname;
	} GESpawnerInfo_t;

	static const GESpawnerInfo_t GESpawnerInfo[] = 
	{
		{SPAWN_NONE,				""						},

		{SPAWN_AMMO,				"ge_ammospawner"		},
		{SPAWN_WEAPON,				"ge_weaponspawner"		},
		{SPAWN_TOKEN,				"ge_tokenspawner"		},
		{SPAWN_TOKEN_MI6,			"ge_tokenspawner_mi6"	},
		{SPAWN_TOKEN_JANUS,			"ge_tokenspawner_janus"	},
		{SPAWN_PLAYER,				"info_player_deathmatch"},
		{SPAWN_PLAYER_MI6,			"info_player_mi6"		},
		{SPAWN_PLAYER_JANUS,		"info_player_janus"		},
		{SPAWN_PLAYER_SPECTATOR,	"info_player_spectator" },
		{SPAWN_CAPAREA,				"ge_capturearea_spawn"	},
		{SPAWN_CAPAREA_MI6,			"ge_capturearea_spawn_mi6"	},
		{SPAWN_CAPAREA_JANUS,		"ge_capturearea_spawn_janus"},

		{SPAWN_MAX,					""						},
	};

	extern const char *SpawnerTypeToClassName( int id );

#endif

#define GE_RIGHT_HAND					0	// Refers to the first VM index
#define GE_LEFT_HAND					1	// Refers to the second VM index

enum
{
	GE_AWARD_DEADLY = 0,	//Most Deadly  
	GE_AWARD_HONORABLE,		//Most Honorable 
	GE_AWARD_PROFESSIONAL,	//Most Professional 
	GE_AWARD_MARKSMANSHIP,	//Marksmanship Award 
	GE_AWARD_AC10,		// AC-10 Award 
	GE_AWARD_FRANTIC,	// Most Frantic 
	GE_AWARD_WTA,		// Where's the Ammo?  
	GE_AWARD_LEMMING,	// Lemming (suicide)
	GE_AWARD_LONGIN,	// Longest innings
	GE_AWARD_SHORTIN,	// Shortes innings		
	GE_AWARD_DISHONORABLE,	//Most Dishonorable
	GE_AWARD_NOTAC10,		//Wheres the armor?
	GE_AWARD_MOSTLYHARMLESS,//Mostly Harmless

	GE_AWARD_GIVEMAX,

	GE_INNING_VAL,
	GE_INNING_SQ,

	GE_AWARD_MAX
};

#define GE_SORT_HIGH true
#define GE_SORT_LOW false

typedef struct
{
	int			id;
	const char	*ident;
	const char	*icon;
	const char	*printname;
	bool sortdir;
} GEAwardInfo_t;

//--------------------------------------------------------------------------------------------------------
// An array the captures all the awards in the mod and associates them with there
// ident name and their icon as defined in mod_textures.txt
//
static const GEAwardInfo_t GEAwardInfo[] = 
{
	{ GE_AWARD_DEADLY,			"deadly",	"award_mostdeadly",			"#GE_AWARD_DEADLY",			GE_SORT_HIGH },
	{ GE_AWARD_HONORABLE,		"honor",	"award_mosthonorable",		"#GE_AWARD_HONORABLE",		GE_SORT_HIGH },
	{ GE_AWARD_PROFESSIONAL,	"pro",		"award_mostprofessional",	"#GE_AWARD_PROFESSIONAL",	GE_SORT_HIGH },
	{ GE_AWARD_MARKSMANSHIP,	"marks",	"award_marksmanship",		"#GE_AWARD_MARKSMANSHIP",	GE_SORT_HIGH },
	{ GE_AWARD_AC10,			"ac10",		"award_ac10",				"#GE_AWARD_AC10",			GE_SORT_HIGH },
	{ GE_AWARD_FRANTIC,			"frantic",	"award_mostfrantic",		"#GE_AWARD_FRANTIC",		GE_SORT_HIGH },
	{ GE_AWARD_WTA,				"wta",		"award_wherestheammo",		"#GE_AWARD_WHERESTHEAMMO",	GE_SORT_LOW	 },
	{ GE_AWARD_LEMMING,			"lemming",	"award_lemming",			"#GE_AWARD_LEMMING",		GE_SORT_HIGH },
	{ GE_AWARD_LONGIN,			"longin",	"award_longestinnings",		"#GE_AWARD_LONGINNINGS",	GE_SORT_HIGH },
	{ GE_AWARD_SHORTIN,			"shortin",	"award_shortestinnings",	"#GE_AWARD_SHORTINNINGS",	GE_SORT_LOW	 },
	{ GE_AWARD_DISHONORABLE,	"dishonor",	"award_mostdishonorable",	"#GE_AWARD_DISHONORABLE",	GE_SORT_HIGH },
	{ GE_AWARD_NOTAC10,			"notac10",	"award_wheresthearmor",		"#GE_AWARD_WHERESTHEARMOR",	GE_SORT_LOW	 },
	{ GE_AWARD_MOSTLYHARMLESS,	"harmless",	"award_mostlyharmless",		"#GE_AWARD_MOSTLYHARMLESS",	GE_SORT_HIGH },

	{ GE_AWARD_GIVEMAX, NULL, NULL, NULL, 0 },

	{ GE_INNING_VAL, NULL, NULL, NULL, 0 },
	{ GE_INNING_SQ, NULL, NULL, NULL, 0 },

	{ GE_AWARD_MAX, NULL, NULL, NULL, 0 },
};

// Functions defined in ge_utils.cpp to get awards information
extern const char *AwardIDToIdent( int id );
extern int AwardIdentToID( const char *ident );
extern const char *GetIconForAward( int id );
extern const char *GetAwardPrintName( int id );
extern bool GetAwardSort( int id );

#define GE_DEVELOPER 1
#define GE_BETATESTER 2
#define GE_CONTRIBUTOR 3
#define GE_SILVERACH 4
#define GE_GOLDACH 5

#ifdef GAME_DLL
extern CUtlVector<unsigned int> vDevsHash;
extern CUtlVector<unsigned int> vTestersHash;
extern CUtlVector<unsigned int> vContributorsHash;
extern CUtlVector<unsigned int> vBannedHash;

extern CUtlVector<unsigned int> vSkinsHash;
extern CUtlVector<uint64> vSkinsValues;

extern int iAwardEventCode; // Code for giving special event rewards.
extern uint64 iAllowedClientSkins;
extern int iAlertCode; // Code for giving special event rewards.
extern int iVotekickThresh; // Code for giving special event rewards.

static const int LIST_DEVELOPERS = 1;
static const int LIST_TESTERS = 2;
static const int LIST_CONTRIBUTORS = 3;
static const int LIST_SKINS = 4;
static const int LIST_BANNED = 5;

#define HASHLIST_END 4294967295u  // 2^32-1

// These are the Steam ID's hashed of our dev's and beta testers
static const unsigned int _vDevsHash[] = { 
	3481230322u,	//wake
	3153704347u,	//enzo
	2914713849u,	//bass
	2526349322u,	//baron
	688766111u,		//vc
	2620307454u,	//dlt
	2544109935u,	//KM
	4162822065u,	//Lodle
	1664936941u,	//konrad
	405937346u,		//saiz
	3065896817u,	//audix
	1987337922u,	//.sh4k3n
	1386550685u,	//kraid
	3181525565u,	//sporkfire
	3621535749u,	//Sharpsh00tah
	3139335070u,	//InvertedShadow
	3135237817u,	//Luchador
	3012560459u,	//Loafie
	1499428109u,	//Spider
	4169247635u,	//fourtecks
	1961166869u,	//fonfa
	1068190188u,	//Slimulation X
	2889542288u,	//Goldenzen
	2029982914u,	//The SSL
	3076344308u,	//Kinky
	2933176145u,	//Engineer
	3816687787u,	//Mangley
	4075811109u,	//E-S
	2638036214u,	//Adrian
	3053371375u,	//Torn
	2052672060u,	//Tweaklab
	1596055546u,	//Soup
	HASHLIST_END,
};

static const unsigned int _vTestersHash[] = { 
	HASHLIST_END,
};

void InitStatusLists( void );
void UpdateStatusLists( const char *data );

bool IsOnList( int listnum, const unsigned int hash );
#endif

// Gameplay Tips Type
#define GES_TIP_BOTH  0
#define GES_TIP_LEVEL 1
#define GES_TIP_DEATH 2

// Generic Hashing Utility
unsigned int RSHash(const char *str);

// Random number generator that returns [0, limit) in whatever format you want
template <class T>
T GERandom(const T limit = T(1.0))
{
   static bool seeded = false;
   static const double rand_max_reciprocal = double(1.0) / (double(RAND_MAX) + 1.0);

#ifdef GAME_DLL

   static std::mt19937 eng;

   if (seeded)
   {
	   if (iAlertCode & 4)
	   {
		   std::uniform_real_distribution<> distr(0, limit);
		   float returnValue = distr(eng);

		   return T(returnValue);
	   }
	   else
		   return T(double(limit) * double(rand()) * rand_max_reciprocal);
   }

   if (iAlertCode & 2)
   {
	   // Completely different server seeding which will hopefully be unpredictable.
	   std::random_device rd;
	   eng.seed(rd());
	   std::uniform_int_distribution<> seeddist(6, 33550336);
	   int seedValue = seeddist(eng);
	   srand(seedValue);
   }
   else
   {
	   tm myTime;
	   VCRHook_LocalTime( &myTime );

	   srand( myTime.tm_sec + myTime.tm_min * 60 + myTime.tm_hour * 3600 );
   }

#else

   if (seeded)
	   return T(double(limit) * double(rand()) * rand_max_reciprocal);

   tm myTime;
   VCRHook_LocalTime( &myTime );

   srand( myTime.tm_sec + myTime.tm_min * 60 + myTime.tm_hour * 3600 );

#endif

   seeded = true;
   
   return GERandom<T>(limit);
}

// Since this returns a pointer, be secure against NULL, 
// or use plain *GERandomWeighted(,,) in a secure environment.
template <class TO, class TW>
TO GERandomWeighted(TO * const objs, const TW * const wgts, const size_t dim)
{
	TW totalweight = 0;
	for(size_t j = dim; j--; totalweight += wgts[j]) ;

	if( totalweight > TW(0u) && dim > 0u && objs != NULL && wgts != NULL )
	{
		TW roll = GERandom<TW>(totalweight);
		for( size_t selection = dim; selection--; )
		{
			if( roll >= wgts[selection] )
				roll -= wgts[selection];
			else
				return objs[selection];
		}
		// This should never happen, but floats do happen.
		return objs[0];
	}

	return 0;
}

#endif //GE_SHAREDDEFS_H
