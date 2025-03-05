#ifndef NEO_CONTROL_POINT_H
#define NEO_CONTROL_POINT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity_shared.h"
#include "baseplayer_shared.h"
#include "neo_gamerules.h"

#ifdef GAME_DLL
#include "entityoutput.h"
#endif

#ifdef CLIENT_DLL
#include "iclientmode.h"
#include "hudelement.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>

#include <vgui/ILocalize.h>
#include "tier3/tier3.h"
#include "vphysics_interface.h"
#include "c_neo_player.h"
#include "ienginevgui.h"
#include "neo_ghost_cap_point.h"
#endif

#ifdef CLIENT_DLL
#define CNEOControlPoint C_NEOControlPoint
class CNEOHud_ControlPoint;
#endif

class CNEOControlPoint : public CBaseEntity
{
	DECLARE_CLASS(CNEOControlPoint, CBaseEntity);

public:
#ifdef GAME_DLL
	DECLARE_SERVERCLASS();
#else
	DECLARE_CLIENTCLASS();
#endif //GAME_DLL
	DECLARE_DATADESC();

	CNEOControlPoint();
	virtual ~CNEOControlPoint();
	bool CreateVPhysics()
	{
		if (!physenv)
		{
			return false;
		}
		objectparams_t params = g_PhysDefaultObjectParams;
		IPhysicsObject* vPhysicsObject = physenv->CreateSphereObject(m_iRadius, 1, GetAbsOrigin(), GetAbsAngles(), &params, true);
		if (vPhysicsObject)
		{
			VPhysicsSetObject(vPhysicsObject);
		}
		SetSolid(SOLID_NONE);
		AddSolidFlags(FSOLID_TRIGGER | FSOLID_USE_TRIGGER_BOUNDS);

		return true;
	};
	
	virtual void Precache(void);
	virtual void Spawn(void);

	int getAttackerTeam() const;
	int getDefenderTeam() const;

	void StartTouch(CBaseEntity* otherEntity) override;
	void EndTouch(CBaseEntity* otherEntity) override;

#ifdef CLIENT_DLL
	virtual void ClientThink(void);
#else
	int UpdateTransmitState() OVERRIDE;
	bool IsGhostCaptured(int &outTeamNumber, int &outCaptorClientIndex);

	void Think(void); // NEO FIXME (Rain): this should be private
#endif // CLIENT_DLL

	void SetActive(bool isActive);
	void ResetCaptureState()
	{
		m_iCaptor = TEAM_INVALID;
		m_flCaptureProgress = 0;
	}

private:
	inline void UpdateVisibility(void);

protected:
#ifdef GAME_DLL
	CNetworkVar(bool, m_bIsActive);
	CNetworkVar(int, m_iCaptor);
	CNetworkVar(float, m_flCaptureProgress);

	CNetworkVar(int, m_iNumJinrai);
	CNetworkVar(int, m_iNumNSF);

	COutputEvent m_OnCapture;
	COutputEvent m_OnPlayerEnter;
	COutputEvent m_OnPlayerLeave;
#else
	bool m_bIsActive;
	int m_iCaptor;
	float m_flCaptureProgress;

	int m_iNumJinrai;
	int m_iNumNSF;

	CNEOHud_GhostCapPoint *m_pHUDControlPoint = nullptr;
#endif // GAME_DLL

	int m_iTouchingPlayers;
	wchar_t m_szName;
	int m_iID;
	int m_iAttackerRequiredControlPoints;
	int m_iDefenderRequiredControlPoints;
	int m_iIcon;
	int m_iRadius;
};

#endif // NEO_CONTROL_POINT_H
