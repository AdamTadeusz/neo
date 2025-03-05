#include "cbase.h"
#include "neo_control_point.h"
#include "neo_player_shared.h"
#include "vphysics_interface.h"
#include "vphysics/friction.h"

#ifdef GAME_DLL
#include "team.h"
#else
#include "c_team.h"
//#include "ui/neo_hud_control_point.h"
#include "ui/neo_hud_ghost_cap_point.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// In seconds, how often should the client think to update capzone graphics info.
// This is *not* the framerate, only info like which team it belongs to, etc.
// Actual rendering happens in the element's Paint() loop.
static constexpr int NEO_CONTROL_POINT_GRAPHICS_THINK_INTERVAL = 1;

static constexpr float NEO_CP_MIN_RADIUS = 8.0f;
static constexpr float NEO_CP_MAX_RADIUS = 1024.0f;

static constexpr int NEO_FGD_TEAMNUM_ATTACKER = 0;
static constexpr int NEO_FGD_TEAMNUM_DEFENDER = 1;

class CNEOControlArea : public CNEOControlPoint
{
public:
	CNEOControlArea() : CNEOControlPoint()
	{
		m_iRadius = 0.f;
	}
};

LINK_ENTITY_TO_CLASS(neo_controlarea, CNEOControlArea);

LINK_ENTITY_TO_CLASS(neo_controlpoint, CNEOControlPoint);

#ifdef GAME_DLL
IMPLEMENT_SERVERCLASS_ST(CNEOControlPoint, DT_NEOControlPoint)
	SendPropBool(SENDINFO(m_bIsActive)),
	SendPropInt(SENDINFO(m_iCaptor)),
	SendPropFloat(SENDINFO(m_flCaptureProgress)),
END_SEND_TABLE()
#else
#ifdef CNEOControlPoint
#undef CNEOControlPoint
#endif
IMPLEMENT_CLIENTCLASS_DT(C_NEOControlPoint, DT_NEOControlPoint, CNEOControlPoint)
	RecvPropInt(RECVINFO(m_bIsActive)),
	RecvPropInt(RECVINFO(m_iCaptor)),
	RecvPropFloat(RECVINFO(m_flCaptureProgress)),
END_RECV_TABLE()
#define CNEOControlPoint C_NEOControlPoint
#endif

BEGIN_DATADESC(CNEOControlPoint)
#ifdef GAME_DLL
	DEFINE_THINKFUNC(Think),
	DEFINE_OUTPUT(m_OnCapture, "OnCapture"),
	DEFINE_OUTPUT(m_OnPlayerEnter, "OnPlayerEnter"),
	DEFINE_OUTPUT(m_OnPlayerLeave, "OnPlayerLeave"),
#endif
	DEFINE_KEYFIELD(m_szName, FIELD_STRING, "Name"),
	DEFINE_KEYFIELD(m_iID, FIELD_INTEGER, "ID"),
	DEFINE_KEYFIELD(m_iAttackerRequiredControlPoints, FIELD_INTEGER, "AttackerRequiredControlPoints"),
	DEFINE_KEYFIELD(m_iDefenderRequiredControlPoints, FIELD_INTEGER, "DefenderRequiredControlPoints"),
	DEFINE_KEYFIELD(m_iIcon, FIELD_INTEGER, "Icon"),
	DEFINE_KEYFIELD(m_iRadius, FIELD_INTEGER, "Radius"),
END_DATADESC()

CNEOControlPoint::CNEOControlPoint()
{
	m_bIsActive = true;
	m_iCaptor = 0;
	m_flCaptureProgress = 0;
#ifdef GAME_DLL
#else
	m_pHUDControlPoint = NULL;
#endif // GAME_DLL
}

CNEOControlPoint::~CNEOControlPoint()
{
#ifdef CLIENT_DLL
	if (m_pHUDControlPoint)
	{
		m_pHUDControlPoint->DeletePanel();
		m_pHUDControlPoint = NULL;
	}
#endif
}

#ifdef GAME_DLL
int CNEOControlPoint::UpdateTransmitState()
{
	return FL_EDICT_ALWAYS;
}
#endif

int CNEOControlPoint::getAttackerTeam() const
{
	return NEORules()->roundAlternate() ? TEAM_JINRAI : TEAM_NSF;
}

int CNEOControlPoint::getDefenderTeam() const
{
	return NEORules()->roundAlternate() ? TEAM_NSF : TEAM_JINRAI;
}
void CNEOControlPoint::Spawn(void)
{
	BaseClass::Spawn();
	CreateVPhysics();
	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);

#ifdef GAME_DLL
	// Warning messages for the about-to-occur clamping, if we've hit limits.
	if (m_iRadius < NEO_CP_MIN_RADIUS)
	{
		Warning("Capzone had too small radius: %f, clamping! (Expected a minimum of %f)\n",
			m_iRadius, NEO_CP_MIN_RADIUS);
	}
	else if (m_iRadius > NEO_CP_MAX_RADIUS)
	{
		Warning("Capzone had too large radius: %f, clamping! (Expected a minimum of %f)\n",
			m_iRadius, NEO_CP_MAX_RADIUS);
	}
	// Actually clamp.
	m_iRadius = clamp(m_iRadius, NEO_CP_MIN_RADIUS, NEO_CP_MAX_RADIUS);

	RegisterThinkContext("Think");
	SetContextThink(&CNEOControlPoint::Think, gpGlobals->curtime, "Think");
#else
	SetNextClientThink(gpGlobals->curtime + NEO_CONTROL_POINT_GRAPHICS_THINK_INTERVAL);
#endif
}

//#define LSBIT(X)                    ((X) & (-(X)))
//#define CLEARLSBIT(X)               ((X) & ((X) - 1))
//
//void CNEOControlPoint::IterateThroughControlPoints()
//{
//	unsigned temp_bits;
//	unsigned one_bit;
//
//	temp_bits = some_value;
//	for (; temp_bits; temp_bits = CLEARLSBIT(temp_bits)) {
//		one_bit = LSBIT(temp_bits);
//		if (m_iAttackerRequiredControlPoints & one_bit)
//		{
//
//		}
//	}
//}

void CNEOControlPoint::StartTouch(CBaseEntity *otherEntity)
{
	if (otherEntity->IsPlayer())
	{
		if (otherEntity->GetTeamNumber() == TEAM_JINRAI)
		{
			m_iNumJinrai++;
		}
		else if (otherEntity->GetTeamNumber() == TEAM_NSF)
		{
			m_iNumNSF++;
		}
	}
}

void CNEOControlPoint::EndTouch(CBaseEntity *otherEntity)
{
	if (otherEntity->IsPlayer())
	{
		if (otherEntity->GetTeamNumber() == TEAM_JINRAI)
		{
			m_iNumJinrai--;
		}
		else if (otherEntity->GetTeamNumber() == TEAM_NSF)
		{
			m_iNumNSF--;
		}
	}
}

#ifdef GAME_DLL
void CNEOControlPoint::Think(void)
{
	// This round has already ended, we can't be capped into
	if (NEORules()->IsRoundOver() || !m_bIsActive)
	{
		return;
	}
	auto controlPoint = this;

	constexpr float TIME_BETWEEN_THINK = 0.1f;

	m_iNumJinrai = m_iNumNSF = 0;
	IPhysicsFrictionSnapshot* pSnapshot = VPhysicsGetObject()->CreateFrictionSnapshot();
	while (pSnapshot->IsValid())
	{
		Vector pt, normal;
		IPhysicsObject* pOther = pSnapshot->GetObject(1);
		CBaseEntity* pEntity0 = static_cast<CBaseEntity*>(pOther->GetGameData());
		if (!pEntity0->IsPlayer() || !pEntity0->IsAlive())
		{
			continue;
		}

		if (pEntity0->GetTeamNumber() == TEAM_JINRAI)
		{
			m_iNumJinrai++;
		}
		else if (pEntity0->GetTeamNumber() == TEAM_NSF)
		{
			m_iNumNSF++;
		}

		pSnapshot->NextFrictionData();
	}
	pSnapshot->DeleteAllMarkedContacts(true);
	VPhysicsGetObject()->DestroyFrictionSnapshot(pSnapshot);

	if (NEORules()->IsRoundLive())
	{
		int teamWithMorePlayers = m_iNumJinrai > m_iNumNSF ? TEAM_JINRAI : m_iNumNSF > m_iNumJinrai ? TEAM_NSF : TEAM_INVALID;
		if (teamWithMorePlayers != TEAM_INVALID)
		{
			m_flCaptureProgress += m_iCaptor == teamWithMorePlayers ? TIME_BETWEEN_THINK : -TIME_BETWEEN_THINK;
		}
		else
		{
			m_flCaptureProgress -= TIME_BETWEEN_THINK;
		}

		if (m_flCaptureProgress <= 0)
		{
			m_flCaptureProgress = 0;
			m_iCaptor = teamWithMorePlayers;
		}
	}
	engine->Con_NPrintf(entindex(), "Owning team: %i, progress: %f", m_iCaptor.Get(), m_flCaptureProgress.Get());

	SetContextThink(&CNEOControlPoint::Think, gpGlobals->curtime + TIME_BETWEEN_THINK, "Think");
}
#else
// Purpose: Set up clientside HUD graphics for capzone.
void CNEOControlPoint::ClientThink(void)
{
	BaseClass::ClientThink();

	// If we haven't set up capzone HUD graphics yet
	if (!m_pHUDControlPoint)
	{
		m_pHUDControlPoint = new CNEOHud_GhostCapPoint("hudCapZone");
	}

	m_pHUDControlPoint->SetPos(GetAbsOrigin());
	m_pHUDControlPoint->SetRadius(64);
	m_pHUDControlPoint->SetTeam(TEAM_NSF);
	m_pHUDControlPoint->SetVisible(true);

	SetNextClientThink(gpGlobals->curtime + 0.1f);
}
#endif

void CNEOControlPoint::Precache(void)
{
	BaseClass::Precache();

	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);
}

void CNEOControlPoint::SetActive(bool isActive)
{
	m_bIsActive = isActive;
	UpdateVisibility();
}

inline void CNEOControlPoint::UpdateVisibility(void)
{
	if (m_bIsActive)
	{
		RemoveEFlags(EF_NODRAW);
	}
	else
	{
		AddEFlags(EF_NODRAW);
	}
}
