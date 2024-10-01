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

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(Map, 0.00695)

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

	if (!(player->m_nButtons & IN_MAP)) // NEOTODO (Adam) Keybind
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

	SetBounds(m_iX0, m_iY0, m_iY1, m_iY1);

	LoadMap();
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

	Vector difference = (Vector(m_flInitialOffsetX, m_flInitialOffsetY, m_flInitialOffsetZ) - playerOrigin);
	constexpr float scaleX = 0.17;
	constexpr float scaleY = 0.17;

	auto playerX = (m_iY1 / 2) + (difference.y * cos(DEG2RAD(35)) * scaleX) + (difference.x * cos(DEG2RAD(55)) * scaleX);
	auto playerY = (m_iY1 / 2) - (difference.y * sin(DEG2RAD(35)) * scaleY) + (difference.x * sin(DEG2RAD(55)) * scaleY) + difference.z * scaleY;

	DrawNeoHudRoundedBox(playerX -5, playerY-5, playerX+5, playerY+5, COLOR_WHITE);
}

