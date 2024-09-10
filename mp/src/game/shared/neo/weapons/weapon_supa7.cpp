#include "cbase.h"
#include "weapon_supa7.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSupa7, DT_WeaponSupa7)

BEGIN_NETWORK_TABLE(CWeaponSupa7, DT_WeaponSupa7)
#ifdef CLIENT_DLL
	RecvPropArray3(RECVINFO_ARRAY(m_iTubeArray), RecvPropInt(RECVINFO(m_iTubeArray [0]))),
	RecvPropInt(RECVINFO(m_iTubeArrayTop)),
	RecvPropInt(RECVINFO(m_iChamber)),
	RecvPropInt(RECVINFO(m_iShellToLoad)),
	RecvPropBool(RECVINFO(m_bJustShot)),
#else
	SendPropArray3(SENDINFO_ARRAY3(m_iTubeArray), SendPropInt(SENDINFO_ARRAY(m_iTubeArray), 10)),
	SendPropInt(SENDINFO(m_iTubeArrayTop)),
	SendPropInt(SENDINFO(m_iChamber)),
	SendPropInt(SENDINFO(m_iShellToLoad)),
	SendPropBool(SENDINFO(m_bJustShot)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSupa7)
	DEFINE_PRED_FIELD(m_iTubeArray, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_iTubeArrayTop, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_iChamber, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_iShellToLoad, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bJustShot, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_supa7, CWeaponSupa7);

PRECACHE_WEAPON_REGISTER(weapon_supa7);

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponSupa7)
	DEFINE_FIELD(m_iTubeArray, FIELD_INTEGER),
	DEFINE_FIELD(m_iTubeArrayTop, FIELD_INTEGER),
	DEFINE_FIELD(m_iChamber, FIELD_INTEGER),
	DEFINE_FIELD(m_iShellToLoad, FIELD_INTEGER),
	DEFINE_FIELD(m_bJustShot, FIELD_BOOLEAN),
END_DATADESC()
#endif

#if(0)
// Might be unsuitable for shell based shotgun? Should sort out the acttable at some point.
NEO_IMPLEMENT_ACTTABLE(CWeaponSupa7)
#else
#ifdef GAME_DLL
acttable_t	CWeaponSupa7::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SHOTGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SHOTGUN, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SHOTGUN, false },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SHOTGUN, false },
};
IMPLEMENT_ACTTABLE(CWeaponSupa7);
#endif
#endif

CWeaponSupa7::CWeaponSupa7(void)
{
	m_bReloadsSingly = true;

	for (int i = 0; i < m_iTubeArray.Count(); ++i)
	{
		m_iTubeArray.Set(i, 0);
	}
	m_iTubeArrayTop = -1;
	m_iChamber = 0;

	m_bStartedReloadingShot = false;
	m_bStartedReloadingSlug = false;
	m_bJustShot = false;

	m_fMinRange1 = 0.0;
	m_fMaxRange1 = 500;
	m_fMinRange2 = 0.0;
	m_fMaxRange2 = 200;
}

void CWeaponSupa7::Spawn(void)
{
	BaseClass::Spawn();
	m_iClip1 = 0;
}

// Purpose: Override so only reload one shell at a time
bool CWeaponSupa7::StartReload(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (m_iTubeArrayTop >= SUPA7_TUBE_SIZE - 1)
		return false;

	if (m_iPrimaryAmmoCount <= 0)
		return false;

	if (!m_bInReload)
	{
		SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);
		m_bInReload = true;
	}

	m_bStartedReloadingShot = true;

	SetShotgunShellVisible(true);

	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());

	return true;
}

// Purpose: Start loading a slug, avoid overriding the default reload functionality due to secondary ammo
bool CWeaponSupa7::StartReloadSlug(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (m_iTubeArrayTop >= SUPA7_TUBE_SIZE - 1)
		return false;

	if (m_iSecondaryAmmoCount <= 0)
		return false;

	if (!m_bInReload)
	{
		SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);
		m_bInReload = true;
	}

	m_bStartedReloadingSlug = true;

	SetShotgunShellVisible(true);

	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());

	return true;
}

// Purpose: Override so only reload one shell at a time
bool CWeaponSupa7::Reload(void)
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Assert(false);
		Warning("ERROR: Supa7 Reload called incorrectly!\n");
	}

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (m_iPrimaryAmmoCount <= 0)
		return false;

	m_iTubeArrayTop++;
	m_iTubeArray.Set(m_iTubeArrayTop, 1);
	m_iClip1++;
	m_iPrimaryAmmoCount--;

	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(SPECIAL1);
	SendWeaponAnim(ACT_VM_RELOAD);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration() - 0.1);

	return true;
}

// Purpose: Start loading a slug, avoid overriding the default reload functionality due to secondary ammo
bool CWeaponSupa7::ReloadSlug(void)
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Assert(false);
		Warning("ERROR: Supa7 ReloadSlug called incorrectly!\n");
	}

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (m_iSecondaryAmmoCount <= 0)
		return false;

	m_iTubeArrayTop++;
	m_iTubeArray.Set(m_iTubeArrayTop, 2);
	m_iClip1++;
	m_iSecondaryAmmoCount--;

	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(SPECIAL3);
	SendWeaponAnim(ACT_VM_RELOAD);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration() - 0.1);
	return true;
}

// Purpose: Play finish reload anim and fill clip
void CWeaponSupa7::FinishReload(void)
{
	SetShotgunShellVisible(false);

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	m_bInReload = false;

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration() - 0.4f);
}

void CWeaponSupa7::PrimaryAttack(void)
{
	if (ShootingIsPrevented())
		return;

	// Only the player fires this way so we can cast
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	if (m_iChamber == 0)
		return;

	pPlayer->ViewPunchReset();

	int numBullets = 7;
	Vector bulletSpread = GetBulletSpread();
	int ammoType = m_iPrimaryAmmoType;
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	// Change the firing characteristics and sound if a slug was loaded
	if (m_iChamber == 2)
	{
		numBullets = 1;
		bulletSpread *= 0.25;
		ammoType = m_iSecondaryAmmoType;
	}
	WeaponSound(SINGLE);

	FireBulletsInfo_t info(numBullets, vecSrc, vecAiming, bulletSpread, MAX_TRACE_LENGTH, ammoType);
	info.m_pAttacker = pPlayer;

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);

	m_iChamber = 0;
	m_bJustShot = true;

	// Don't fire again until fire animation has completed
	ProposeNextAttack(gpGlobals->curtime + GetFireRate());

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Fire the bullets, and force the first shot to be perfectly accurate
	pPlayer->FireBullets(info);

	if (!m_iClip1 && m_iPrimaryAmmoCount <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
	AddViewKick();
}

void CWeaponSupa7::SecondaryAttack(void)
{
	if (ShootingIsPrevented())
		return;

	StartReloadSlug();
}

// Purpose: Override so shotgun can do multiple reloads in a row
void CWeaponSupa7::ItemPostFrame(void)
{
	auto pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	ProcessAnimationEvents();

	if (m_bInReload)
	{
		if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			if (m_bStartedReloadingShot)
			{
				m_bStartedReloadingShot = false;
				Reload();
				return;
			}
			if (m_bStartedReloadingSlug)
			{
				m_bStartedReloadingSlug = false;
				ReloadSlug();
				return;
			}

			if (pOwner->m_nButtons & IN_RELOAD && m_iTubeArrayTop < SUPA7_TUBE_SIZE - 1 && m_iPrimaryAmmoCount > 0)
			{
				Reload();
				return;
			}
			else if (pOwner->m_nButtons & IN_ATTACK2 && m_iTubeArrayTop < SUPA7_TUBE_SIZE - 1 && m_iSecondaryAmmoCount > 0)
			{
				ReloadSlug();
				return;
			}

			FinishReload();
			return;
		}
	}
	else
	{
		SetShotgunShellVisible(false);
	}

	if (pOwner->m_nButtons & IN_RELOAD)
	{
		if (m_bInReload)
			return;

		if (m_iTubeArrayTop >= SUPA7_TUBE_SIZE - 1 || m_iPrimaryAmmoCount <= 0)
		{
			SendWeaponAnim(ACT_VM_IDLE);
			return;
		}

		StartReload();
		return;
	}
	else if (pOwner->m_nButtons & IN_ATTACK2)
	{
		if (m_bInReload)
			return;

		if (m_iTubeArrayTop >= SUPA7_TUBE_SIZE - 1 || m_iSecondaryAmmoCount <= 0)
		{
			SendWeaponAnim(ACT_VM_IDLE);
			return;
		}

		StartReloadSlug();
		return;
	}
	else if (pOwner->m_nButtons & IN_ATTACK && m_flNextPrimaryAttack - 0.3f <= gpGlobals->curtime && !m_iChamber && m_iTubeArrayTop >= 0)
	{
		m_iChamber = m_iTubeArray[m_iTubeArrayTop];
		m_iTubeArrayTop--;
		m_iClip1--;
		WeaponSound(SPECIAL2);
		SendWeaponAnim(ACT_SHOTGUN_PUMP);
		m_flNextPrimaryAttack = (gpGlobals->curtime + SequenceDuration());
		return;
	}
	else if (pOwner->m_nButtons & IN_ATTACK && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		if (!m_iChamber && m_iTubeArrayTop == -1)
		{
			DryFire();
			return;
		}

		if (pOwner->GetWaterLevel() == WL_Eyes && !m_bFiresUnderwater)
		{
			WeaponSound(EMPTY);
			ProposeNextAttack(gpGlobals->curtime + GetFastestDryRefireTime());
			return;
		}
		// Fire underwater?
		else
		{
			// If the firing button was just pressed, reset the firing time
			CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
			if (pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK)
			{
				ProposeNextAttack(gpGlobals->curtime);
			}
			PrimaryAttack();
		}
	}

	if (!HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime)
	{
		// weapon isn't useable, switch.
		if (!(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pOwner->SwitchToNextBestWeapon(this))
		{
			ProposeNextAttack(gpGlobals->curtime + 0.3);
			return;
		}
	}

	if (m_bJustShot && m_flNextPrimaryAttack - 0.7f < gpGlobals->curtime)
	{
		if (pOwner->m_nButtons & IN_ATTACK)
		{
			m_bJustShot = false;
			if (m_iTubeArrayTop >= 0)
			{
				m_iChamber = m_iTubeArray[m_iTubeArrayTop];
				m_iTubeArrayTop--;
				m_iClip1--;
				WeaponSound(SPECIAL2);
				return;
			}
		}
		SendWeaponAnim(ACT_VM_IDLE);
		m_bJustShot = false;
		return;
	}

	WeaponIdle();
}

void CWeaponSupa7::AddViewKick(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	QAngle punch;
	punch.Init(SharedRandomFloat("supapax", -2, -1), SharedRandomFloat("supapay", -1, 1), 0);
	pPlayer->ViewPunch(punch);
}

bool CWeaponSupa7::SlugLoaded() const
{
	return m_iChamber == 2;
}

bool CWeaponSupa7::ShotLoaded() const
{
	return m_iChamber == 1;
}

void CWeaponSupa7::Drop(const Vector& vecVelocity)
{
	CNEOBaseCombatWeapon::Drop(vecVelocity);
}
