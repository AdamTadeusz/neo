#include "cbase.h"
#include "neo_hud_map.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IScheme.h>

#include "ienginevgui.h"
#include "filesystem.h"

#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_Map, NHudMap);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(Map, 0.1)

CNEOHud_Map::CNEOHud_Map(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), EditablePanel(parent, pElementName)
{
	SetAutoDelete(true);

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	m_hPlayerTexture = surface()->CreateNewTextureID();
	Assert(m_hPlayerTexture > 0);
	surface()->DrawSetTextureFile(m_hPlayerTexture, "maps/playerMapIcon", 1, false);

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);

	m_hFont = 0;
	SetVisible(true);
}

void CNEOHud_Map::UpdateStateForNeoHudElementDraw()
{
}


void CNEOHud_Map::LoadMap()
{
	constexpr char ROOT_MAP_FOLDER[] = "maps/minimaps";
	char mapInfoFolder[128] = {};
	char mapName[128] = {};
	auto mapNameLength = V_sprintf_safe(mapName, "%s", engine->GetLevelName()) - 4;
	if (mapNameLength < 0)
	{
		return;
	}
	mapName[mapNameLength] = '\0';
	V_sprintf_safe(mapInfoFolder, "%s/%s", mapName, "map_info.txt");
	
	KeyValues* script = new KeyValues("minimapInfo");
#ifndef _XBOX
	if (script->LoadFromFile(filesystem, mapInfoFolder))
#else
	if (filesystem->LoadKeyValues(*script, IFileSystem::TYPE_SOUNDSCAPE, mapInfoFolder, "GAME"))
#endif
	{
		m_mapInfo.RemoveAll();
		// parse out all of the top level sections and save their names
		KeyValues* pKeys = script;
		pKeys = pKeys->GetFirstSubKey();
		// map name = pKeys->GetString()
		pKeys = pKeys->GetNextKey();
		m_flInitialOffsetX = pKeys->GetFloat();
		pKeys = pKeys->GetNextKey();
		m_flInitialOffsetY = pKeys->GetFloat();
		pKeys = pKeys->GetNextKey();
		m_flInitialOffsetZ = pKeys->GetFloat();
		pKeys = pKeys->GetNextKey();
		m_flCameraAngle = pKeys->GetFloat();
		pKeys = pKeys->GetNextKey();
		m_flOrthographicScale = pKeys->GetFloat() * (103.f/35050.f);
		pKeys = pKeys->GetNextKey();
		m_iNumLevels = pKeys->GetInt();
		for (int i = 0; i < m_iNumLevels; i++)
		{
			pKeys = pKeys->GetNextKey();
			if (!pKeys || i >= MAX_NUM_LEVELS)
			{
				break;
			}
			m_iLevelMaxes[i] = pKeys->GetFloat();
			m_hLevels[i] = surface()->CreateNewTextureID();
			V_sprintf_safe(mapInfoFolder, "%s/level%i", mapName, i);
			surface()->DrawSetTextureFile(m_hLevels[i], mapInfoFolder, 1, false);
		}
	}
	else
	{
		script->deleteThis();
	}
}

void CNEOHud_Map::Paint()
{
	PaintNeoElement();

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	BaseClass::Paint();
}

void CNEOHud_Map::DrawNeoHudElement(void)
{
	if (!ShouldDraw())
	{
		return;
	}

	auto player = C_NEO_Player::GetLocalNEOPlayer();
	if (!player)
	{
		return;
	}

	if (!(player->m_nButtons & IN_MAP))
	{
		return;
	}

	if (m_showMap)
	{
		DrawMap();
	}
}

void CNEOHud_Map::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("scripts/HudLayout.res");

	m_hFont = pScheme->GetFont("NHudOCRSmall");

	surface()->GetScreenSize(m_resX, m_resY);

	m_iY0 = m_resY / 8;
	m_iY1 = m_iY0 * 6;
	m_iX0 = (m_resX / 2) - (m_iY0 * 3);
	m_iX1 = (m_resX / 2) + (m_iY0 * 3);

	m_flAngle = atan(sin(DEG2RAD(m_flCameraAngle)) * (sin(DEG2RAD(m_flCameraAngle)) / cos(DEG2RAD(m_flCameraAngle))));
	m_flCos = cos(m_flAngle);
	m_flSin = sin(m_flAngle);
	m_flScale = ((float)m_resY / 1200.f) * m_flOrthographicScale;

	SetBounds(m_iX0, m_iY0, m_iY1, m_iY1);

	LoadMap();
}

void CNEOHud_Map::DrawPlayer(C_NEO_Player *player, Color colour) const
{
	Vector difference = (Vector(m_flInitialOffsetX, m_flInitialOffsetY, m_flInitialOffsetZ) - player->GetAbsOrigin());
	auto playerX = (m_iY1 / 2) + (difference.y * m_flCos * m_flScale) + (difference.x * m_flCos * m_flScale);
	auto playerY = (m_iY1 / 2) - (difference.y * m_flSin * m_flScale) + (difference.x * m_flSin * m_flScale) + difference.z * m_flScale;

	surface()->DrawSetTexture(m_hPlayerTexture);
	surface()->DrawSetColor(colour);
	surface()->DrawTexturedRect(playerX - 5, playerY - 5, playerX + 5, playerY + 5);

	constexpr int LINE_SIZE = 20;
	float directionY = player->GetAbsAngles().y + 180;
	int viewX1 = LINE_SIZE * sin(DEG2RAD(directionY));
	int viewY1 = LINE_SIZE * cos(DEG2RAD(directionY));
	int viewX2 = LINE_SIZE * sin(DEG2RAD(directionY) + m_flAngle*2);
	int viewY2 = LINE_SIZE * cos(DEG2RAD(directionY) + m_flAngle*2);

	surface()->DrawLine(playerX, playerY, playerX + viewX1, playerY + viewY1);
	surface()->DrawLine(playerX, playerY, playerX + viewX2, playerY + viewY2);
}

void CNEOHud_Map::DrawMap() const
{
	auto player = C_NEO_Player::GetLocalNEOPlayer();
	Assert(player);

	surface()->DrawSetTextFont(m_hFont);
	DrawNeoHudRoundedBox(0, 0, m_iY1, m_iY1, m_boxColor);
	
	Vector playerOrigin = player->GetAbsOrigin();
	
	int currentLevel;
	for (currentLevel = 0; currentLevel < m_iNumLevels; currentLevel++)
	{
		if (playerOrigin.z > m_iLevelMaxes[currentLevel])
		{
			continue;
		}
		break;
	}

	if (currentLevel - 1 >= 0)
	{
		surface()->DrawSetTexture(m_hLevels[currentLevel - 1]);
		surface()->DrawSetColor(COLOR_FADED_WHITE);
		surface()->DrawTexturedRect(0, 0, m_iY1, m_iY1);
	}
	surface()->DrawSetTexture(m_hLevels[currentLevel]);
	surface()->DrawSetColor(COLOR_WHITE);
	surface()->DrawTexturedRect(0, 0, m_iY1, m_iY1);
	if (currentLevel + 1 < m_iNumLevels)
	{
		surface()->DrawSetTexture(m_hLevels[currentLevel + 1]);
		surface()->DrawSetColor(COLOR_FADED_WHITE);
		surface()->DrawTexturedRect(0, 0, m_iY1, m_iY1);
	}

	int localPlayerIndex = GetLocalPlayerIndex();
	int localPlayerTeam = GetLocalPlayerTeam();
	
	if (player->IsAlive())
	{
		DrawPlayer(player, COLOR_YELLOW);
	}

	if (localPlayerTeam != TEAM_NSF)
	{
		C_Team *teamJinrai = GetGlobalTeam(TEAM_JINRAI);
		auto memberCount = teamJinrai->GetNumPlayers();
		for (int i = 0; i < memberCount; ++i)
		{
			auto player = static_cast<C_NEO_Player*>(teamJinrai->GetPlayer(i));
			if (player && localPlayerIndex != player->entindex() && player->IsAlive())
			{
				DrawPlayer( player, COLOR_JINRAI);
			}
		}
	}

	if (localPlayerTeam != TEAM_JINRAI)
	{
		C_Team* teamNsf = GetGlobalTeam(TEAM_NSF);
		auto memberCount = teamNsf->GetNumPlayers();
		for (int i = 0; i < memberCount; ++i)
		{
			auto player = static_cast<C_NEO_Player*>(teamNsf->GetPlayer(i));
			if (player && localPlayerIndex != player->entindex() && player->IsAlive())
			{
				DrawPlayer(player, COLOR_NSF);
			}
		}
	}
}

