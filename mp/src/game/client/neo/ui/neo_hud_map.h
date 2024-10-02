#ifndef NEO_HUD_MAP_H
#define NEO_HUD_MAP_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

class CNEOHud_Map : public CNEOHud_ChildElement, public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Map, EditablePanel);

public:
	CNEOHud_Map(const char *pElementName, vgui::Panel *parent = nullptr);
	virtual void FireGameEvent(IGameEvent* event) override;
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	void LoadMap();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void DrawPlayer(C_NEO_Player* player, Color colour) const;
	void DrawMap() const;

private:
	vgui::HTexture m_hPlayerTexture = 0UL;

	int m_resX, m_resY, m_iX0, m_iY0, m_iY1;
	float m_flCameraAngle;
	float m_flOrthographicScale;
	float m_flAngle;
	float m_flCos;
	float m_flSin;
	float m_flScale;

	float m_flInitialOffsetX = 0.0f;
	float m_flInitialOffsetY = 0.0f;
	float m_flInitialOffsetZ = 0.0f;
	int m_iNumLevels = 0;
	static constexpr int MAX_NUM_LEVELS = 8;
	float m_iLevelMaxes[MAX_NUM_LEVELS] = {};
	vgui::HTexture m_hLevels[MAX_NUM_LEVELS];
	CUtlVector<KeyValues*>		m_mapInfo;

	CPanelAnimationVarAliasType(bool, m_showMap, "visible", "1", "bool");
	CPanelAnimationVar(Color, m_boxColor, "box_color", "200 200 200 40");


private:
	CNEOHud_Map(const CNEOHud_Map &other);
};

#endif // NEO_HUD_MAP_H
