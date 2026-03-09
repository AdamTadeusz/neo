#pragma once

class IMaterial;


class CThermopticHandler
{
public:
	CThermopticHandler();

public:
	IMaterial* GetThermopticMaterial( float fNewTransparency = -1.f );

	void UpdateThermopticMaterial( float fNewTransparency );
	
private:
	bool CreateThermopticMaterial();

private:
	IMaterial* m_pThermMaterial;
	float m_fTransparency;
};

extern CThermopticHandler* g_ThermopticHandler;
