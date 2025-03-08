#include "neo_gamerules.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#include "team.h"
#endif // CLIENT_DLL

struct NeoGameTypeSettings
{
	const char* name;
	const char* acronym;
	const char* (*getObjective)(CNEO_Player* neoPlayer);
	const bool neoRulesThink;
	const bool comp;
	const bool capPrevent;
	bool (*canRespawn)(CNEO_Player* neoPlayer);
	bool (*canChangeTeamClassLoadout)(CNEO_Player* neoPlayer);
};
bool boo() { return false; };
const char* chara() { return "Nothing"; };
int inte() { return 0; };

////////////////
// Objectives //
////////////////
inline const char* getObjectivePoints(CNEO_Player* neoPlayer) { return "Score the most points"; };
inline const char* getObjectiveCTG(CNEO_Player* neoPlayer) { return "Capture the ghost"; };
inline const char* getObjectiveVIP(CNEO_Player* neoPlayer) { 
#ifdef CLIENT_DLL
	return NEORules()->m_iEscortingTeam == neoPlayer->GetTeamNumber() ? "Escort the VIP" : "Eliminate the VIP"; 
#else
	return "";
#endif // CLIENT_DLL
};
inline const char* getObjectiveNothing(CNEO_Player* neoPlayer) { return "Await further instructions"; };
inline const char* getObjectiveTutorial(CNEO_Player* neoPlayer) { return "Proceed through the training"; };

/////////////////
// Can Respawn //
/////////////////
inline bool canXAlways(CNEO_Player* neoPlayer) { return true; };
extern ConVar mp_neo_latespawn_max_time;
inline bool canRespawnOneLifePerRound(CNEO_Player* neoPlayer) {
#ifdef CLIENT_DLL
	return false;
#else
	if (!NEORules()->IsRoundOn())
	{
		return true;
	}
	return !(neoPlayer->m_bSpawnedThisRound || NEORules()->GetRoundAccumulatedTime() > mp_neo_latespawn_max_time.GetFloat() || neoPlayer->m_bKilledInflicted);
#endif // CLIENT_DLL
};
inline bool canXIfDead(CNEO_Player* neoPlayer) { 
#ifdef CLIENT_DLL
	return false;
#else
	return neoPlayer->IsDead();
#endif // CLIENT_DLL
};

/////////////////////////////////
// Can Change Loadout or Class //
/////////////////////////////////
inline bool canXIfDeadOrPreRound(CNEO_Player* neoPlayer) {
#ifdef CLIENT_DLL
	return false;
#else
	return neoPlayer->IsDead() || !NEORules()->IsRoundOn();
#endif // CLIENT_DLL
};

extern ConVar sv_neo_time_alive_until_cant_change_loadout;
inline bool canChangeTeamClassLoadoutIfEligible(CNEO_Player* neoPlayer) {
#ifdef CLIENT_DLL
	return false;
#else
	if (NEORules()->IsRoundOn() && 
		(neoPlayer->IsObserver() || neoPlayer->IsDead() || neoPlayer->m_bIneligibleForLoadoutPick || 
			neoPlayer->m_aliveTimer.IsGreaterThen(sv_neo_time_alive_until_cant_change_loadout.GetFloat())))
	{
		return false;
	}
	return true;
#endif // CLIENT_DLL
};

///////////////////////////////
// Check winning team/player //
///////////////////////////////
inline void checkWinnerTDM()
{
#ifdef CLIENT_DLL
#else
	if (GetGlobalTeam(TEAM_JINRAI)->GetScore() > GetGlobalTeam(TEAM_NSF)->GetScore())
	{
		NEORules()->SetWinningTeam(TEAM_JINRAI, NEO_VICTORY_POINTS, false, true, false, false);
		return;
	}

	if (GetGlobalTeam(TEAM_NSF)->GetScore() > GetGlobalTeam(TEAM_JINRAI)->GetScore())
	{
		NEORules()->SetWinningTeam(TEAM_NSF, NEO_VICTORY_POINTS, false, true, false, false);
		return;
	}

	NEORules()->SetWinningTeam(TEAM_SPECTATOR, NEO_VICTORY_STALEMATE, false, true, false, false);
#endif // CLIENT_DLL
};

inline void checkWinnerDM()
{
#ifdef CLIENT_DLL
#else
	// Winning player
	CNEO_Player* pWinners[MAX_PLAYERS + 1] = {};
	int iWinnersTotal = 0;
	int iWinnerXP = 0;
	NEORules()->GetDMHighestScorers(&pWinners, &iWinnersTotal, &iWinnerXP);
	if (iWinnersTotal == 1)
	{
		NEORules()->SetWinningDMPlayer(pWinners[0]);
		return;
	}

	// Otherwise go into overtime
#endif // CLIENT_DLL
};

inline void checkWinnerCTG()
{
#ifdef CLIENT_DLL
#else

#endif // CLIENT_DLL
}

const NeoGameTypeSettings NEO_GAME_TYPE_SETTINGS[NEO_GAME_TYPE__TOTAL] = {
/*						name				acronym		getObjective			neoRulesThink	comp	capPrevent	canRespawn					canChangeTeamClassLoadout*/
/*NEO_GAME_TYPE_TDM*/ {"Team Deathmatch",	"TDM",		getObjectivePoints,		true,			false,	false,		canXAlways,					canChangeTeamClassLoadoutIfEligible},
/*NEO_GAME_TYPE_CTG*/ {"Capture the Ghost", "CTG",		getObjectiveCTG,		true,			true,	false,		canRespawnOneLifePerRound,	canChangeTeamClassLoadoutIfEligible},
/*NEO_GAME_TYPE_VIP*/ {"VIP",				"VIP",		getObjectiveVIP,		true,			true,	false,		canRespawnOneLifePerRound,	canChangeTeamClassLoadoutIfEligible},
/*NEO_GAME_TYPE_DM*/  {"Deathmatch",		"DM",		getObjectivePoints,		true,			false,	false,		canXAlways,					canChangeTeamClassLoadoutIfEligible},
/*NEO_GAME_TYPE_EMT*/ {"Empty",				"EMT",		getObjectiveNothing,	false,			false,	false,		canXAlways,					canXAlways},
/*NEO_GAME_TYPE_TUT*/ {"Tutorial",			"TUT",		getObjectiveTutorial,	false,			false,	false,		canXIfDead,					canXIfDead},
};