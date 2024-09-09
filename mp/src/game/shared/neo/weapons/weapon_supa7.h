#ifndef NEO_WEAPON_SUPA7_H
#define NEO_WEAPON_SUPA7_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponSupa7 C_WeaponSupa7
#endif

#define SUPA7_TUBE_SIZE 6

class CWeaponSupa7 : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponSupa7, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponSupa7();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_SUPA7; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 140.0 / 170.0; }

	virtual int GetMinBurst() OVERRIDE { return 1; }
	virtual int GetMaxBurst() OVERRIDE { return 3; }

	bool StartReload(void);
	bool StartReloadSlug(void);
	bool Reload(void);
	bool ReloadSlug(void);
	bool SlugLoaded(void) const;

	void FinishReload(void);
	void AddViewKick(void);
	void ItemPostFrame(void);
	void PrimaryAttack(void);
	void SecondaryAttack(void);

	void Drop(const Vector& vecVelocity) OVERRIDE;

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	// Purpose: Only update next attack time if it's further away in the future.
	void ProposeNextAttack(const float flNextAttackProposal)
	{
		if (m_flNextPrimaryAttack < flNextAttackProposal)
		{
			m_flNextPrimaryAttack = flNextAttackProposal;
		}
	}

	void SetShotgunShellVisible(bool is_visible)
	{
		const int bodygroup_shell = 1;
		const int shell_visible = 0, shell_invisible = 1;
		SetBodygroup(bodygroup_shell, is_visible ? shell_visible : shell_invisible);
	}

private:
	CNetworkArray(int, m_iTubeArray, SUPA7_TUBE_SIZE); // The supa7 has a tube size of 6 plus one in the chamber
	CNetworkVar(int, m_iTubeArrayTop); // Index of the first round to be loaded from the tube
	CNetworkVar(int, m_iChamber); // The shot/ or lack thereof, currently in the chamber 
	CNetworkVar(int, m_iShellToLoad); // Whether we are reloading Slug or Shot
	CNetworkVar(bool, m_bStartedReloadingShot);
	CNetworkVar(bool, m_bStartedReloadingSlug);
	CNetworkVar(bool, m_bJustShot);

private:
	CWeaponSupa7(const CWeaponSupa7 &other);
};

#endif // NEO_WEAPON_SUPA7_H
