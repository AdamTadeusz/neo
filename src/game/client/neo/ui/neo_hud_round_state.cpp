#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ImageList.h>
#include "neo_hud_round_state.h"

#include "iclientmode.h"
#include "ienginevgui.h"
#include "engine/IEngineSound.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include <initializer_list>

#include <vgui_controls/ImagePanel.h>

#include "c_neo_player.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "vgui_avatarimage.h"
#include "neo_scoreboard.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_RoundState, NRoundState);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(RoundState, 0.1)

ConVar cl_neo_hud_team_swap_sides("cl_neo_hud_team_swap_sides", "1", FCVAR_ARCHIVE, "Make the team of the local player always appear on the left side of the round info and scoreboard", true, 0.0, true, 1.0,
	[]([[maybe_unused]] IConVar* var, [[maybe_unused]] const char* pOldValue, [[maybe_unused]] float flOldValue) {
		g_pNeoScoreBoard->UpdateTeamColumnsPosition(GetLocalPlayerTeam());
	});
ConVar cl_neo_squad_hud_original("cl_neo_squad_hud_original", "1", FCVAR_ARCHIVE, "Use the old squad HUD", true, 0.0, true, 1.0,
	[]([[maybe_unused]] IConVar* var, [[maybe_unused]] const char* pOldString, [[maybe_unused]] float flOldValue)->void{
		CNEOHud_RoundState* roundStateHudElement = GET_NAMED_HUDELEMENT(CNEOHud_RoundState, NRoundState);
		if (!roundStateHudElement)
		{
			return;
		}
		roundStateHudElement->m_bSquadHudTypeMaybeChanged = true;
	}
);
ConVar cl_neo_squad_hud_star_scale("cl_neo_squad_hud_star_scale", "0", FCVAR_ARCHIVE, "Scaling to apply from 1080p, 0 disables scaling");
extern ConVar sv_neo_dm_win_xp;
extern ConVar cl_neo_streamermode;
extern ConVar snd_victory_volume;
extern ConVar sv_neo_readyup_countdown;
extern ConVar cl_neo_hud_scoreboard_hide_others;
extern ConVar sv_neo_ctg_ghost_overtime_grace;
extern ConVar cl_neo_hud_health_mode;

namespace {
constexpr bool STARS_HW_FILTERED = false;
}

CNEOHud_RoundState::CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName)
	, Panel(parent, pElementName)
{
	m_pWszStatusUnicode = L"";
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_ROUND_STATE;

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	// Initialize star textures
	const bool starAutoDelete = true;
	COMPILE_TIME_ASSERT(starAutoDelete); // If not, we need to handle that memory on dtor manually

	for (int i = 0; i < STAR__TOTAL; ++i)
	{
		static constexpr const char *IP_STAR_NAMES[STAR__TOTAL] = {
			"none", "alpha", "bravo", "charlie", "delta", "echo", "foxtrot"
		};
		const char *name = IP_STAR_NAMES[i];
		char ipName[32];
		char hudName[32];
		V_snprintf(ipName, sizeof(ipName), "star_%s", name);
		V_snprintf(hudName, sizeof(hudName), "hud/star_%s", name);

		m_ipStars[i] = new vgui::ImagePanel(this, ipName);
		auto *star = m_ipStars[i];
		star->SetImage(vgui::scheme()->GetImage(hudName, STARS_HW_FILTERED));
		star->SetDrawColor(COLOR_NSF); // This will get updated in the draw check as required
		star->SetAlpha(1.0f);
		star->SetShouldScaleImage(true);
		star->SetAutoDelete(starAutoDelete);
		star->SetVisible(false);
	}

	for (int i = 0; i < NEO_CLASS__ENUM_COUNT; ++i)
	{
		static constexpr const char *TEX_NAMES[NEO_CLASS__ENUM_COUNT] = {
			"vgui/reconSmall", "vgui/assaultSmall", "vgui/supportSmall", "vgui/vipSmall", "vgui/vipSmall"
		};
		m_iGraphicID[i] = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile(m_iGraphicID[i], TEX_NAMES[i], true, false);
	}

	struct TeamLogoColorInfo
	{
		const char *logo;
		const char *totalLogo;
		Color color;
	};
	for (int i = FIRST_GAME_TEAM; i < TEAM__TOTAL; ++i)
	{
		static const TeamLogoColorInfo TEAM_TEX_INFO[TEAM__TOTAL - FIRST_GAME_TEAM] = {
			{.logo = "vgui/jinrai_128tm", .totalLogo = "vgui/ts_jinrai", .color = COLOR_JINRAI},
			{.logo = "vgui/nsf_128tm", .totalLogo = "vgui/ts_nsf", .color = COLOR_NSF},
		};
		const int texIdx = i - FIRST_GAME_TEAM;
		m_teamLogoColors[i] = TeamLogoColor{
			.logo = surface()->CreateNewTextureID(),
			.totalLogo = surface()->CreateNewTextureID(),
			.color = TEAM_TEX_INFO[texIdx].color,
		};
		surface()->DrawSetTextureFile(m_teamLogoColors[i].logo, TEAM_TEX_INFO[texIdx].logo, true, false);
		surface()->DrawSetTextureFile(m_teamLogoColors[i].totalLogo, TEAM_TEX_INFO[texIdx].totalLogo, true, false);
	}

}

void CNEOHud_RoundState::LevelShutdown(void)
{
	for (int i = 0; i < STAR__TOTAL; ++i)
	{
		auto* star = m_ipStars[i];
		star->SetVisible(false);
	}
}

void CNEOHud_RoundState::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hOCRLargeFont = pScheme->GetFont("NHudOCRLarge");
	m_hOCRFont = pScheme->GetFont("NHudOCR");
	m_hOCRSmallFont = pScheme->GetFont("NHudOCRSmall");
	m_hOCRSmallerFont = pScheme->GetFont("NHudOCRSmaller");

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	// Screen dimensions
	IntDim res = {};
	surface()->GetScreenSize(res.w, res.h);
	m_iXpos = (res.w / 2);

	if (cl_neo_squad_hud_star_scale.GetFloat())
	{
		const float scale = cl_neo_squad_hud_star_scale.GetFloat() * (res.h / 1080.0);
		for (auto* star : m_ipStars)
		{
			star->SetWide(192 * scale);
			star->SetTall(48 * scale);
		}
	}
	else {
		for (auto* star : m_ipStars)
		{
			star->SetWide(192);
			star->SetTall(48);
		}
	}

	// Box dimensions
	[[maybe_unused]] int iSmallFontWidth = 0;
	int iFontHeight = 0;
	int iFontWidth = 0;
	surface()->GetTextSize(m_hOCRSmallFont, L"ROUND 99", iSmallFontWidth, m_iSmallFontHeight);
	surface()->GetTextSize(m_hOCRFont, L"ROUND 99", iFontWidth, iFontHeight);
	m_iSmallFontHeight *= 0.85;
	iFontHeight *= 0.85;

	const int iBoxHeight = iFontHeight + m_iSmallFontHeight * 2 + 1;
	const int iBoxHeightHalf = (iBoxHeight / 2);
	const int iBoxWidth = iSmallFontWidth + iBoxHeight * 2;
	const int iBoxWidthHalf = (iBoxWidth / 2);
	m_iLeftOffset = m_iXpos - iBoxWidthHalf;
	m_iRightOffset = m_iXpos + iBoxWidthHalf;
	m_iBoxYEnd = m_iYPos + iBoxHeight;
	m_ilogoSize = 32;

	m_rectLeftTeamNumberLogo = vgui::IntRect{
		.x0 = m_iLeftOffset,
		.y0 = m_iYPos,
		.x1 = m_iLeftOffset + iBoxHeight,
		.y1 = m_iYPos + iBoxHeight,
	};
	m_rectRightTeamNumberLogo = vgui::IntRect{
		.x0 = m_iRightOffset - iBoxHeight,
		.y0 = m_iYPos,
		.x1 = m_iRightOffset,
		.y1 = m_iYPos + iBoxHeight,
	};

	surface()->GetTextSize(m_hOCRLargeFont, L"ROUND 99", iFontWidth, iFontHeight);
	m_posLeftTeamNumber = IntPos{
		.x = m_rectLeftTeamNumberLogo.x0 + iBoxHeightHalf,
		.y = static_cast<int>(m_iYPos + iBoxHeightHalf - (iFontHeight / 2)),
	};
	m_posRightTeamNumber = IntPos{
		.x = m_rectRightTeamNumberLogo.x0 + iBoxHeightHalf,
		.y = static_cast<int>(m_iYPos + iBoxHeightHalf - (iFontHeight / 2)),
	};

	SetBounds(0, 0, res.w, res.h);
	SetZPos(90);
}

extern ConVar sv_neo_readyup_lobby;

void CNEOHud_RoundState::UpdateStateForNeoHudElementDraw()
{
	float roundTimeLeft = NEORules()->GetRoundRemainingTime();
	const NeoRoundStatus roundStatus = NEORules()->GetRoundStatus();
	const bool inSuddenDeath = NEORules()->RoundIsInSuddenDeath();
	const bool inMatchPoint = NEORules()->RoundIsMatchPoint();

	m_pWszStatusUnicode = L"";
	if (roundStatus == NeoRoundStatus::Idle)
	{
		m_pWszStatusUnicode = sv_neo_readyup_lobby.GetBool() ? L"Waiting for players to ready up" : L"Waiting for players";
	}
	else if (roundStatus == NeoRoundStatus::Warmup)
	{
		m_pWszStatusUnicode = L"Warmup";
	}
	else if (roundStatus == NeoRoundStatus::Countdown)
	{
		m_pWszStatusUnicode = L"Match is starting...";
	}
	else if (inSuddenDeath)
	{
		m_pWszStatusUnicode = L"Sudden death";
	}
	else if (inMatchPoint)
	{
		m_pWszStatusUnicode = L"Match point";
	}
	else if (NEORules()->GetRoundStatus() != NeoRoundStatus::Pause && (NEORules()->IsRoundPreRoundFreeze() || g_pNeoScoreBoard->IsVisible()))
	{
		// Update Objective
		switch (NEORules()->GetGameType()) {
		case NEO_GAME_TYPE_DM:
			// Don't print objective for deathmatch
			m_pWszStatusUnicode = L"\0";
			break;
		case NEO_GAME_TYPE_TDM:
			m_pWszStatusUnicode = L"Score the most Points\n";
			break;
		case NEO_GAME_TYPE_CTG:
			m_pWszStatusUnicode = L"Capture the Ghost\n";
			break;
		case NEO_GAME_TYPE_VIP:
			if (GetLocalPlayerTeam() == NEORules()->m_iEscortingTeam.Get())
			{
				if (NEORules()->GhostExists())
				{
					m_pWszStatusUnicode = L"VIP down, prevent Ghost capture\n";
				}
				else
				{
					m_pWszStatusUnicode = L"Escort the VIP\n";
				}
			}
			else
			{
				if (NEORules()->GhostExists())
				{
					m_pWszStatusUnicode = L"HVT down, secure the Ghost\n";
				}
				else
				{
					m_pWszStatusUnicode = L"Eliminate the HVT\n";
				}
			}
			break;
		case NEO_GAME_TYPE_JGR:
			m_pWszStatusUnicode = L"Control the Juggernaut\n";
			break;
		default:
			m_pWszStatusUnicode = L"Await further orders\n";
			break;
		}
	}
	m_iStatusUnicodeSize = V_wcslen(m_pWszStatusUnicode);

	// Clear the strings so zero roundTimeLeft also picks it up as to not draw
	memset(m_wszRoundUnicode, 0, sizeof(m_wszRoundUnicode));
	memset(m_wszLeftNumber, 0, sizeof(m_wszLeftNumber));
	memset(m_wszRightNumber, 0, sizeof(m_wszRightNumber));
	memset(m_wszTime, 0, sizeof(m_wszTime));
	memset(m_wszLeftTeamRoundsWon, 0, sizeof(m_wszLeftTeamRoundsWon));
	memset(m_wszRightTeamRoundsWon, 0, sizeof(m_wszRightTeamRoundsWon));
	memset(m_wszRoundsDraw, 0, sizeof(m_wszRoundsDraw));

	// Exactly zero means there's no time limit, so we don't need to draw anything.
	if (roundTimeLeft == 0)
	{
		return;
	}
	// Less than 0 means round is over, but cap the timer to zero for nicer display.
	else if (roundTimeLeft < 0)
	{
		roundTimeLeft = 0;
	}

	if (NEORules()->GetRoundStatus() == NeoRoundStatus::Pause)
	{
		m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"PAUSED");
		roundTimeLeft = NEORules()->m_flPauseEnd.Get() - gpGlobals->curtime;
	}
	else if (NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown)
	{
		m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"STARTING");
	}
	else if (NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime)
	{
		m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"OVERTIME");
	}
	else
	{
		if (NEORules()->GetGameType() == NEO_GAME_TYPE_DM)
		{
			m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"DEATHMATCH");
		}
		else
		{
			m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"ROUND %i", NEORules()->roundNumber());
		}
	}

	if (roundStatus == NeoRoundStatus::PreRoundFreeze)
		roundTimeLeft = NEORules()->GetRemainingPreRoundFreezeTime(true);

	int secsTotal = 0.0f;
	if (roundStatus == NeoRoundStatus::Overtime && NEORules()->GetGameType() == NEO_GAME_TYPE_CTG)
	{
		secsTotal = RoundFloatToInt(NEORules()->GetCTGOverTime());
	}
	else
	{
		secsTotal = RoundFloatToInt(roundTimeLeft);
	}

	const int secsRemainder = secsTotal % 60;
	const int minutes = (secsTotal - secsRemainder) / 60;
	V_snwprintf(m_wszTime, 6, L"%02d:%02d", minutes, secsRemainder);

	int iDMHighestXP = 0;

	const int localPlayerTeam = GetLocalPlayerTeam();
	const bool localPlayerSpecOrNoTeam = !NEORules()->IsTeamplay() || !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);
	bool swapTeamSides = cl_neo_hud_team_swap_sides.GetBool();
	const int leftTeam = swapTeamSides ? (localPlayerSpecOrNoTeam ? TEAM_JINRAI : localPlayerTeam) : TEAM_JINRAI;
	if (NEORules()->IsTeamplay())
	{
		V_snwprintf(m_wszLeftNumber, ARRAYSIZE(m_wszLeftNumber), L"%i", NEORules()->CanRespawnAnyTime() ? GetGlobalTeam(leftTeam)->Get_Score() : m_iLeftPlayersAlive);
		V_snwprintf(m_wszRightNumber, ARRAYSIZE(m_wszRightNumber), L"%i", NEORules()->CanRespawnAnyTime() ? GetGlobalTeam(NEORules()->GetOpposingTeam(leftTeam))->Get_Score()  : m_iRightPlayersAlive);

		char szRoundsANSI[ARRAYSIZE(m_wszRoundsDraw)] = {};
		COMPILE_TIME_ASSERT(ARRAYSIZE(m_wszRoundsDraw) >= ARRAYSIZE(m_wszLeftTeamRoundsWon));
		COMPILE_TIME_ASSERT(ARRAYSIZE(m_wszRoundsDraw) >= ARRAYSIZE(m_wszRightTeamRoundsWon));
		const int leftTeamRoundsWon = GetGlobalTeam(leftTeam)->GetRoundsWon();
		const int rightTeamRoundsWon = GetGlobalTeam(NEORules()->GetOpposingTeam(leftTeam))->GetRoundsWon();
		const int roundsDrawn = NEORules()->roundNumber() - leftTeamRoundsWon - rightTeamRoundsWon - (roundStatus == NeoRoundStatus::PostRound ? 0 : 1);
		V_snwprintf(m_wszLeftTeamRoundsWon, ARRAYSIZE(m_wszLeftTeamRoundsWon), L"%i ", leftTeamRoundsWon);
		V_snwprintf(m_wszRightTeamRoundsWon, ARRAYSIZE(m_wszRightTeamRoundsWon), L" %i", rightTeamRoundsWon);
		V_snwprintf(m_wszRoundsDraw, ARRAYSIZE(m_wszRoundsDraw), L": %i :", roundsDrawn);
	}

	C_NEO_Player* localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (localPlayer)
	{

		if (NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown)
		{
			// NEO NOTE (nullsystem): Keep secs in integer and previous round state
			// so it beeps client-side in sync with time change
			if (m_ePrevRoundStatus != NeoRoundStatus::Countdown)
			{
				m_iBeepSecsTotal = sv_neo_readyup_countdown.GetInt();
			}

			if (m_iBeepSecsTotal != secsTotal)
			{
				const bool bEndBeep = secsTotal == 0;
				const float flVol = (bEndBeep) ? (1.3f * snd_victory_volume.GetFloat()) : snd_victory_volume.GetFloat();
				static constexpr int PITCH_END = 165;
				enginesound->EmitAmbientSound("tutorial/hitsound.wav", flVol, bEndBeep ? PITCH_END : PITCH_NORM);
				m_iBeepSecsTotal = secsTotal;
			}
			// Otherwise don't beep/alter m_iBeepSecsTotal
		}
	}

	m_ePrevRoundStatus = NEORules()->GetRoundStatus();
}

void CNEOHud_RoundState::DrawNeoHudElement()
{
	CheckActiveStar();

	if (!ShouldDraw())
	{
		return;
	}

	surface()->DrawSetTextFont(m_hOCRFont);
	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hOCRFont, m_wszTime, fontWidth, fontHeight);

	// Draw Box
	DrawNeoHudRoundedBox(m_iLeftOffset, m_iYPos, m_iRightOffset, m_iBoxYEnd, box_color, top_left_corner, top_right_corner, bottom_left_corner, bottom_right_corner);

	// Draw round
	surface()->DrawSetTextColor((NEORules()->GetRoundStatus() == NeoRoundStatus::Pause) ? COLOR_RED : COLOR_FADED_WHITE);
	surface()->DrawSetTextFont(m_hOCRSmallerFont);
	surface()->GetTextSize(m_hOCRSmallerFont, m_wszRoundUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iYPos);
	surface()->DrawPrintText(m_wszRoundUnicode, m_iWszRoundUCSize);

	// Draw round status
	surface()->DrawSetTextColor(COLOR_WHITE);
	surface()->DrawSetTextFont(m_hOCRSmallFont);
	surface()->GetTextSize(m_hOCRSmallFont, m_pWszStatusUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iBoxYEnd);
	surface()->DrawPrintText(m_pWszStatusUnicode, m_iStatusUnicodeSize);

	const int localPlayerTeam = GetLocalPlayerTeam();
	const int localPlayerIndex = GetLocalPlayerIndex();
	const bool localPlayerSpecOrNoTeam = !NEORules()->IsTeamplay() || !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

	bool swapTeamSides = cl_neo_hud_team_swap_sides.GetBool();
	const int leftTeam = swapTeamSides ? (localPlayerSpecOrNoTeam ? TEAM_JINRAI : localPlayerTeam) : TEAM_JINRAI;
	const int rightTeam = swapTeamSides ? ((leftTeam == TEAM_JINRAI) ? TEAM_NSF : TEAM_JINRAI) : TEAM_NSF;
	const auto leftTeamInfo = m_teamLogoColors[leftTeam];
	const auto rightTeamInfo = m_teamLogoColors[rightTeam];

	// Draw game score
	surface()->DrawSetTextFont(m_hOCRSmallerFont);
	surface()->GetTextSize(m_hOCRSmallerFont, m_wszRoundsDraw, fontWidth, fontHeight);
	int offset = fontWidth / 2;
	surface()->DrawSetTextColor(COLOR_FADED_WHITE);
	surface()->DrawSetTextPos(m_iXpos - offset, m_iBoxYEnd - fontHeight);
	surface()->DrawPrintText(m_wszRoundsDraw, ARRAYSIZE(m_wszRoundsDraw) - 1);

	surface()->GetTextSize(m_hOCRSmallerFont, m_wszLeftTeamRoundsWon, fontWidth, fontHeight);
	surface()->DrawSetTextColor(leftTeamInfo.color);
	surface()->DrawSetTextPos(m_iXpos - offset - fontWidth, m_iBoxYEnd - fontHeight);
	surface()->DrawPrintText(m_wszLeftTeamRoundsWon, ARRAYSIZE(m_wszLeftTeamRoundsWon) - 1);

	surface()->GetTextSize(m_hOCRSmallerFont, m_wszRightTeamRoundsWon, fontWidth, fontHeight);
	surface()->DrawSetTextColor(rightTeamInfo.color);
	surface()->DrawSetTextPos(m_iXpos + offset, m_iBoxYEnd - fontHeight);
	surface()->DrawPrintText(m_wszRightTeamRoundsWon, ARRAYSIZE(m_wszRightTeamRoundsWon) - 1);

	// Draw time
	constexpr const int SEC_OFFSET = 3;
	constexpr const int SPLIT_OFFSET = 2;
	surface()->DrawSetTextFont(m_hOCRFont);
	int secWidth;
	surface()->GetTextSize(m_hOCRFont, &m_wszTime[SEC_OFFSET], secWidth, fontHeight);
	int splitWidth;
	surface()->GetTextSize(m_hOCRFont, &m_wszTime[2], fontWidth, fontHeight);
	splitWidth = fontWidth - secWidth;
	surface()->GetTextSize(m_hOCRFont, m_wszTime, fontWidth, fontHeight);
	fontWidth = fontWidth - splitWidth - secWidth;
	if (NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze || NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown || NEORules()->GetGameType() == NEO_GAME_TYPE_CTG ?
		(NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime && (NEORules()->GetRoundRemainingTime() < sv_neo_ctg_ghost_overtime_grace.GetFloat())) : NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime)
	{
		surface()->DrawSetTextColor(COLOR_RED);
	}
	else
	{
		surface()->DrawSetTextColor(COLOR_WHITE);
	}
	const int iBoxYMiddle = m_iYPos + ((m_iBoxYEnd - m_iYPos) / 2);
	surface()->DrawSetTextPos(m_iXpos + (splitWidth / 2), iBoxYMiddle - (fontHeight / 2));
	surface()->DrawPrintText(&m_wszTime[SEC_OFFSET], ARRAYSIZE(m_wszTime) - SEC_OFFSET - 1);
	surface()->DrawSetTextPos(m_iXpos - (splitWidth / 2), iBoxYMiddle - (fontHeight / 2));
	surface()->DrawPrintText(&m_wszTime[SPLIT_OFFSET], SEC_OFFSET - SPLIT_OFFSET);
	surface()->DrawSetTextPos(m_iXpos - fontWidth - (splitWidth / 2), iBoxYMiddle - (fontHeight / 2));
	surface()->DrawPrintText(m_wszTime, ARRAYSIZE(m_wszTime) - 1 - 2 - 1);

	if (NEORules()->IsTeamplay())
	{
		// Draw team logo
		surface()->DrawSetTexture(leftTeamInfo.totalLogo);
		surface()->DrawSetColor(COLOR_FADED_DARK);
		surface()->DrawTexturedRect(m_rectLeftTeamNumberLogo.x0,
									m_rectLeftTeamNumberLogo.y0,
									m_rectLeftTeamNumberLogo.x1,
									m_rectLeftTeamNumberLogo.y1);

		surface()->DrawSetTexture(rightTeamInfo.totalLogo);
		surface()->DrawTexturedRect(m_rectRightTeamNumberLogo.x0,
									m_rectRightTeamNumberLogo.y0,
									m_rectRightTeamNumberLogo.x1,
									m_rectRightTeamNumberLogo.y1);

		// Draw num players alive (or round score)
		surface()->GetTextSize(m_hOCRLargeFont, m_wszLeftNumber, fontWidth, fontHeight);
		surface()->DrawSetTextFont(m_hOCRLargeFont);
		surface()->DrawSetTextPos(m_posLeftTeamNumber.x - (fontWidth / 2), m_posLeftTeamNumber.y);
		surface()->DrawSetTextColor(leftTeamInfo.color);
		surface()->DrawPrintText(m_wszLeftNumber, 2);
	
		surface()->GetTextSize(m_hOCRLargeFont, m_wszRightNumber, fontWidth, fontHeight);
		surface()->DrawSetTextFont(m_hOCRLargeFont);
		surface()->DrawSetTextPos(m_posRightTeamNumber.x - (fontWidth / 2), m_posRightTeamNumber.y);
		surface()->DrawSetTextColor(rightTeamInfo.color);
		surface()->DrawPrintText(m_wszRightNumber, 2);
	}

	if (!g_PR)
	{
		return;
	}

	if (cl_neo_squad_hud_original.GetBool())
	{
		DrawPlayerList();
	}
	else
	{
		DrawPlayerIcons();
	}
}

void CNEOHud_RoundState::DrawPlayerList()
{
	// Draw members of players squad in an old style list
	const int localPlayerTeam = GetLocalPlayerTeam();
	const int localPlayerIndex = GetLocalPlayerIndex();
	const bool localPlayerSpec = !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);
	const int leftTeam = localPlayerSpec ? TEAM_JINRAI : localPlayerTeam;

	int offset = 52;
	if (cl_neo_squad_hud_star_scale.GetFloat() > 0)
	{
		IntDim res = {};
		surface()->GetScreenSize(res.w, res.h);
		offset *= cl_neo_squad_hud_star_scale.GetFloat() * res.h / 1080.0f;
	}

	const bool hideDueToScoreboard = cl_neo_hud_scoreboard_hide_others.GetBool() && g_pNeoScoreBoard->IsVisible();

	// Draw squad mates
	if (!localPlayerSpec && g_PR->GetStar(localPlayerIndex) != 0 && !hideDueToScoreboard)
	{
		bool squadMateFound = false;

		for (int i = 0; i < (MAX_PLAYERS + 1); i++)
		{
			if (i == localPlayerIndex)
			{
				continue;
			}
			if (!g_PR->IsConnected(i))
			{
				continue;
			}
			const int playerTeam = g_PR->GetTeam(i);
			if (playerTeam != leftTeam)
			{
				continue;
			}
			const bool isSameSquad = g_PR->GetStar(i) == g_PR->GetStar(localPlayerIndex);
			if (!isSameSquad)
			{
				continue;
			}

			offset = DrawPlayerRow(i, offset);
			squadMateFound = true;
		}

		if (squadMateFound)
		{
			offset += 12;
		}
	}

	m_iLeftPlayersAlive = 0;
	m_iRightPlayersAlive = 0;

	// Draw other team mates
	for (int i = 0; i < (MAX_PLAYERS + 1); i++)
	{
		if (!g_PR->IsConnected(i))
		{
			continue;
		}
		const int playerTeam = g_PR->GetTeam(i);
		if (playerTeam != leftTeam)
		{
			if (g_PR->IsAlive(i)) 
			{
				m_iRightPlayersAlive++;
			}
			continue;
		}
		else {
			if (g_PR->IsAlive(i))
			{
				m_iLeftPlayersAlive++;
			}
		}
		if (i == localPlayerIndex || localPlayerSpec || hideDueToScoreboard)
		{
			continue;
		}
		const bool isSameSquad = g_PR->GetStar(i) == g_PR->GetStar(localPlayerIndex);
		if (isSameSquad)
		{
			continue;
		}

		offset = DrawPlayerRow(i, offset, true);
	}
}

int CNEOHud_RoundState::DrawPlayerRow(int playerIndex, const int yOffset, bool small)
{
	// Draw player
	static constexpr int SQUAD_MATE_TEXT_LENGTH = 62; // 31 characters in name without end character max plus 3 in short rank name plus 7 max in class name plus 3 max in health plus other characters
	char squadMateText[SQUAD_MATE_TEXT_LENGTH];
	wchar_t wSquadMateText[SQUAD_MATE_TEXT_LENGTH];
	const char* squadMateRankName = GetRankName(g_PR->GetXP(playerIndex), true);
	const char* squadMateClass = GetNeoClassName(g_PR->GetClass(playerIndex));
	const bool isAlive = g_PR->IsAlive(playerIndex);

	if (isAlive)
	{
		const int healthMode = cl_neo_hud_health_mode.GetInt();
		char playerHealth[7]; // 4 digits + 2 letters
		V_snprintf(playerHealth, sizeof(playerHealth), healthMode ? "%dhp" : "%d%%", g_PR->GetDisplayedHealth(playerIndex, healthMode));
		V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s %s  [%s]  %s", g_PR->GetPlayerName(playerIndex), squadMateRankName, squadMateClass, playerHealth);
	}
	else
	{
		V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s  [%s]  DEAD", g_PR->GetPlayerName(playerIndex), squadMateClass);
	}
	g_pVGuiLocalize->ConvertANSIToUnicode(squadMateText, wSquadMateText, sizeof(wSquadMateText));

	int fontWidth, fontHeight;
	surface()->DrawSetTextFont(small ? m_hOCRSmallerFont : m_hOCRSmallFont);
	surface()->GetTextSize(m_hOCRSmallFont, m_wszLeftTeamRoundsWon, fontWidth, fontHeight);
	surface()->DrawSetTextColor(isAlive ? COLOR_FADED_WHITE : COLOR_DARK_FADED_WHITE);
	surface()->DrawSetTextPos(8, yOffset);
	surface()->DrawPrintText(wSquadMateText, V_wcslen(wSquadMateText));

	return yOffset + fontHeight;
}

void CNEOHud_RoundState::DrawPlayerIcons()
{
	m_iLeftPlayersAlive = 0;
	m_iLeftPlayersTotal = 0;
	m_iRightPlayersAlive = 0;
	m_iRightPlayersTotal = 0;
	int leftCount = 0;
	int rightCount = 0;
	bool bDMRightSide = false;

	const int localPlayerTeam = GetLocalPlayerTeam();
	const int localPlayerIndex = GetLocalPlayerIndex();
	const bool localPlayerSpecOrNoTeam = !NEORules()->IsTeamplay() || !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

	bool swapTeamSides = cl_neo_hud_team_swap_sides.GetBool();
	const int leftTeam = swapTeamSides ? (localPlayerSpecOrNoTeam ? TEAM_JINRAI : localPlayerTeam) : TEAM_JINRAI;
	const int rightTeam = swapTeamSides ? ((leftTeam == TEAM_JINRAI) ? TEAM_NSF : TEAM_JINRAI) : TEAM_NSF;
	const auto leftTeamInfo = m_teamLogoColors[leftTeam];
	const auto rightTeamInfo = m_teamLogoColors[rightTeam];

	if (NEORules()->IsTeamplay())
	{
		CUtlVectorFixed<CUtlVector<int>, NeoStar::STAR__TOTAL> stars;
		stars.AddMultipleToTail(NeoStar::STAR__TOTAL);
		for (int i = 0; i < (MAX_PLAYERS + 1); i++)
		{
			if (g_PR->IsConnected(i))
			{
				const int playerTeam = g_PR->GetTeam(i);
				if (playerTeam == leftTeam)
				{
					if (g_PR->IsAlive(i))
						m_iLeftPlayersAlive++;
					m_iLeftPlayersTotal++;

					const int squad = g_PR->GetStar(i);
					stars[squad].AddToTail(i);
				}
				else if (playerTeam == rightTeam)
				{
					const int xOffset = (m_iRightOffset + 2) + (rightCount * m_ilogoSize) + (rightCount * 2);
					DrawPlayer(i, rightCount, rightTeamInfo, xOffset, localPlayerSpecOrNoTeam);
					rightCount++;

					if (g_PR->IsAlive(i))
						m_iRightPlayersAlive++;
					m_iRightPlayersTotal++;
				}
			}
		}
		
		int xOffset = m_iLeftOffset - 1;
		for (int star : {NeoStar::STAR_ALPHA, NeoStar::STAR_BRAVO, NeoStar::STAR_CHARLIE, NeoStar::STAR_DELTA, NeoStar::STAR_ECHO, NeoStar::STAR_FOXTROT, NeoStar::STAR_NONE})
		{
			if (stars[star].IsEmpty())
			{
				continue;
			}

			const bool isSameSquad = g_PR->GetStar(localPlayerIndex) == star;
			int i = 0;
			bool hasLivePlayer = false;
			for (i = 0; i < stars[star].Count(); i++)
			{
				xOffset = (xOffset - m_ilogoSize) - 2;
				DrawPlayer(stars[star][i], leftCount, leftTeamInfo, xOffset, true);
				leftCount++;

				if (g_PR->IsAlive(stars[star][i]))
				{
					hasLivePlayer = true;
				}
			}
			wchar_t wSquadlabel[12];
			switch (star) {
				case NeoStar::STAR_ALPHA:
					g_pVGuiLocalize->ConvertANSIToUnicode(i >= 2 ? "ALPHA" : "A", wSquadlabel, sizeof(wSquadlabel));
					break;
				case NeoStar::STAR_BRAVO:
					g_pVGuiLocalize->ConvertANSIToUnicode(i >= 2 ? "BRAVO" : "B", wSquadlabel, sizeof(wSquadlabel));
					break;
				case NeoStar::STAR_CHARLIE:
					g_pVGuiLocalize->ConvertANSIToUnicode(i >= 3 ? "CHARLIE" : i >= 2 ? "CHARL" : "C", wSquadlabel, sizeof(wSquadlabel));
					break;
				case NeoStar::STAR_DELTA:
					g_pVGuiLocalize->ConvertANSIToUnicode(i >= 2 ? "DELTA" : "D", wSquadlabel, sizeof(wSquadlabel));
					break;
				case NeoStar::STAR_ECHO:
					g_pVGuiLocalize->ConvertANSIToUnicode(i >= 2 ? "ECHO" : "E", wSquadlabel, sizeof(wSquadlabel));
					break;
				case NeoStar::STAR_FOXTROT:
					g_pVGuiLocalize->ConvertANSIToUnicode(i >= 3 ? "FOXTROT" : i >= 2 ? "FOXTR" : "F", wSquadlabel, sizeof(wSquadlabel));
					break;
				default:
					continue;
			}

			int fontWidth, fontHeight;
			surface()->DrawSetTextFont(m_hOCRSmallerFont);
			surface()->GetTextSize(m_hOCRSmallerFont, wSquadlabel, fontWidth, fontHeight);
			
			DrawNeoHudRoundedBox(xOffset - 1, m_iYPos + m_ilogoSize + 6 + 2, xOffset + fontWidth + 5, m_iYPos + m_ilogoSize + 6 + 2 + fontHeight, box_color, false, false, true, true);

			surface()->DrawSetTextColor(isSameSquad ? leftTeamInfo.color : hasLivePlayer ? COLOR_FADED_WHITE : COLOR_DARK_FADED_WHITE);
			surface()->DrawSetTextPos(xOffset + 2, m_iYPos + m_ilogoSize + 6 + 2);
			surface()->DrawPrintText(wSquadlabel, V_wcslen(wSquadlabel));

			if (!hasLivePlayer) {
				const int ypos = m_iYPos + m_ilogoSize + 6 + 2;
				surface()->DrawSetColor(COLOR_WHITE);
				surface()->DrawLine(xOffset, ypos + (fontHeight * 0.25), xOffset + 2 + fontWidth, ypos + (fontHeight * 0.75) );
			}

			xOffset -= (m_ilogoSize / 6);
		}
	}
	else
	{
		PlayerXPInfo playersOrder[MAX_PLAYERS + 1] = {};
		int iTotalPlayers = 0;
		DMClSortedPlayers(&playersOrder, &iTotalPlayers);

		// Second pass: Render the players in this order
		const int iLTRSwitch = Ceil2Int(iTotalPlayers / 2.0f);
		leftCount = (iLTRSwitch - 1); // Start from furthest leftCount index from the center
		rightCount = 0;
		bool bOnLeft = true;
		for (int i = 0; i < iTotalPlayers; ++i)
		{
			if (i == iLTRSwitch)
			{
				bOnLeft = false;
			}

			const int iPlayerIdx = playersOrder[i].idx;
			const bool bPlayerLocal = g_PR->IsLocalPlayer(iPlayerIdx);

			// NEO NOTE (nullsystem): Even though they can be Jinrai/NSF, at most it's just a skin and different
			// color in non-teamplay deathmatch mode.
			const auto lrTeamInfo = (g_PR->GetTeam(iPlayerIdx) == leftTeam) ? leftTeamInfo : rightTeamInfo;
			if (bOnLeft)
			{
				const int xOffset = (m_iLeftOffset - 2) - ((leftCount + 1) * m_ilogoSize) - (leftCount * 2);
				DrawPlayer(iPlayerIdx, leftCount, lrTeamInfo, xOffset, bPlayerLocal);
				--leftCount;
			}
			else
			{
				const int xOffset = (m_iRightOffset + 2) + (rightCount  * m_ilogoSize) + (rightCount * 2);
				DrawPlayer(iPlayerIdx, rightCount, lrTeamInfo, xOffset, bPlayerLocal);
				rightCount++;
			}
		}
	}
}

void CNEOHud_RoundState::DrawPlayer(int playerIndex, int teamIndex, const TeamLogoColor &teamLogoColor,
									const int xOffset, const bool drawHealthClass)
{
	const int ypos = m_iYPos;
	// Draw Outline
	surface()->DrawSetColor(box_color);
	surface()->DrawFilledRect(xOffset - 1, ypos, xOffset + m_ilogoSize + 1,
							  ypos + m_ilogoSize + 2 + (drawHealthClass ? 5 : 0));

	// Drawing Avatar
	surface()->DrawSetTexture(teamLogoColor.logo);

	if (g_PR->IsAlive(playerIndex)) {
		surface()->DrawSetColor(teamLogoColor.color);
		surface()->DrawFilledRect(xOffset, ypos + 1, xOffset + m_ilogoSize, ypos + m_ilogoSize + 1);
	}
	else {
		surface()->DrawSetColor(COLOR_DARK);
		surface()->DrawFilledRect(xOffset, ypos + 1, xOffset + m_ilogoSize, ypos + m_ilogoSize + 1);
	}

	SetTextureToAvatar(playerIndex);
	if (!g_PR->IsAlive(playerIndex))
		surface()->DrawSetColor(COLOR_DARK);
	surface()->DrawTexturedRect(xOffset, ypos + 1, xOffset + m_ilogoSize, ypos + m_ilogoSize + 1);

	// Deathmatch only: Draw XP on everyone
	if (!NEORules()->IsTeamplay())
	{
		wchar_t wszXP[9];
		const int iWszLen = V_swprintf_safe(wszXP, L"%d", g_PR->GetXP(playerIndex));
		int fontWidth, fontHeight;
		surface()->DrawSetTextFont(m_hOCRSmallFont);
		surface()->GetTextSize(m_hOCRSmallFont, wszXP, fontWidth, fontHeight);
		const int iYExtra = drawHealthClass ? m_ilogoSize : fontHeight;
		surface()->DrawSetTextPos(xOffset + ((m_ilogoSize / 2) - (fontWidth / 2)),
								  ypos + m_ilogoSize + 2 - (fontHeight / 2) + iYExtra);
		surface()->DrawSetTextColor(COLOR_WHITE);
		surface()->DrawPrintText(wszXP, iWszLen);
	}

	// Return early to not draw healthbar and class icon
	if (!drawHealthClass)
	{
		return;
	}

	// Drawing Healthbar
	if (!g_PR->IsAlive(playerIndex))
		return;

	const int health = g_PR->GetDisplayedHealth(playerIndex, 0);
	if (health_monochrome) {
		const int greenBlueValue = (health / 100.0f) * 255;
		surface()->DrawSetColor(Color(255, greenBlueValue, greenBlueValue, 255));
	}
	else {
		if (health <= 20)
			surface()->DrawSetColor(COLOR_RED);
		else if (health <= 80)
			surface()->DrawSetColor(COLOR_YELLOW);
		else
			surface()->DrawSetColor(COLOR_WHITE);
	}
	surface()->DrawFilledRect(xOffset, ypos + m_ilogoSize + 2, xOffset + (health / 100.0f * m_ilogoSize), ypos + m_ilogoSize + 6);

	// Drawing Class Icon
	/*surface()->DrawSetTexture(m_iGraphicID[g_PR->GetClass(playerIndex)]);
	surface()->DrawSetColor(teamLogoColor.color);
	surface()->DrawTexturedRect(xOffset, ypos + m_ilogoSize + 7, xOffset + m_ilogoSize, ypos + m_ilogoSize + 71);*/
}

void CNEOHud_RoundState::CheckActiveStar()
{
	auto player = C_NEO_Player::GetLocalNEOPlayer();
	Assert(player);

	int currentStar;
	if (!ShouldDraw())
	{
		currentStar = STAR_NONE;
	}
	else
	{
		currentStar = player->GetStar();
	}
	const int currentTeam = player->GetTeamNumber();

	if (m_iPreviouslyActiveStar == currentStar && m_iPreviouslyActiveTeam == currentTeam && !m_bSquadHudTypeMaybeChanged)
	{
		return;
	}

	m_iPreviouslyActiveStar = currentStar;
	m_iPreviouslyActiveTeam = currentTeam;
	m_bSquadHudTypeMaybeChanged = false;

	for (auto *ipStar : m_ipStars)
	{
		ipStar->SetVisible(false);
	}

	if (currentTeam != TEAM_JINRAI && currentTeam != TEAM_NSF || !cl_neo_squad_hud_original.GetBool())
	{
		return;
	}

	// NEO NOTE (nullsystem): Unlikely, but sanity checking
	Assert(currentStar >= 0 && currentStar < STAR__TOTAL);
	if (currentStar < 0 || currentStar >= STAR__TOTAL)
	{
		currentStar = STAR_NONE;
	}

	vgui::ImagePanel *target = m_ipStars[currentStar];
	target->SetVisible(true);
	target->SetDrawColor(currentStar == STAR_NONE ? COLOR_NEO_WHITE : currentTeam == TEAM_NSF ? COLOR_NSF : COLOR_JINRAI);
}

void CNEOHud_RoundState::SetTextureToAvatar(int playerIndex)
{
	if (!g_pNeoScoreBoard)
	{
		return;
	}

	if (cl_neo_streamermode.GetBool())
	{
		return;
	}

	player_info_t pi;
	if (!engine->GetPlayerInfo(playerIndex, &pi))
		return;

	if (!pi.friendsID)
		return;

	CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
	const int mapIndex = g_pNeoScoreBoard->m_mapAvatarsToImageList.Find(steamIDForPlayer);
	if ((mapIndex == g_pNeoScoreBoard->m_mapAvatarsToImageList.InvalidIndex()))
		return;

	CAvatarImage* pAvIm = (CAvatarImage*)g_pNeoScoreBoard->m_pImageList->GetImage(g_pNeoScoreBoard->m_mapAvatarsToImageList[mapIndex]);
	surface()->DrawSetTexture(pAvIm->getTextureID());
	surface()->DrawSetColor(COLOR_WHITE);
}

void CNEOHud_RoundState::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}
