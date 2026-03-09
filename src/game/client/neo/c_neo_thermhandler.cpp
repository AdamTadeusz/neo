#include "cbase.h"
#include "c_neo_thermhandler.h"
#include "materialsystem\imaterialvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CThermopticHandler* g_ThermopticHandler = nullptr;

CThermopticHandler::CThermopticHandler()
{
	m_pThermMaterial = nullptr;
	m_fTransparency = 0.0f;
	g_ThermopticHandler = this;
	CreateThermopticMaterial();
}

IMaterial* CThermopticHandler::GetThermopticMaterial( float fNewTransparency )
{
	if (fNewTransparency >= 0.f)
		UpdateThermopticMaterial(fNewTransparency);
	
	m_pThermMaterial = materials->FindMaterial("models/player/thermoptic", TEXTURE_GROUP_MODEL);
	return m_pThermMaterial;
}

void CThermopticHandler::UpdateThermopticMaterial( float fNewTransparency )	
{
	// When standing still, refracttint = [0.85, 0.85, 0.85], refractamount = [0.035, 0.035]
	// At full speed, refracttint = [0.69, 0.73, 0.73], refractamount = [0.235, 0.235]
	m_fTransparency = fNewTransparency * 0.2f; 
	
	m_pThermMaterial = materials->FindMaterial("models/player/thermoptic", TEXTURE_GROUP_MODEL);

	bool found = false;

	IMaterialVar* refractamount = m_pThermMaterial->FindVar( "$refractamount", &found );

	if ( found )
		refractamount->SetFloatValue( m_fTransparency + 0.035f );

	IMaterialVar* bluramount = m_pThermMaterial->FindVar( "$bluramount", &found );

	if ( found )
		bluramount->SetFloatValue( m_fTransparency * 20.0f + 10.5f );

	IMaterialVar* refracttint = m_pThermMaterial->FindVar( "$refracttint", &found );

	if ( found )
		refracttint->SetVecValue( 0.85f - m_fTransparency * 0.8f, 0.85f - m_fTransparency * 0.6f, 0.85f -m_fTransparency * 0.6f );

	IMaterialVar* fresnelreflection = m_pThermMaterial->FindVar( "$fresnelreflection", &found );

	if ( found )
		fresnelreflection->SetFloatValue( m_fTransparency + m_fTransparency + 0.25f );

	/*IMaterialVar* bumptransform = m_pThermMaterial->FindVar( "$bumptransform", &found );

	if ( found )
	{
		VMatrix matrix( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );

		if ( bumptransform->GetType() == MATERIAL_VAR_TYPE_MATRIX )
			bumptransform->SetMatrixValue( matrix );
		else
			bumptransform->SetVecValue( 0.0f, 0.0f, 0.0f );
	}*/
}

bool CThermopticHandler::CreateThermopticMaterial()
{
	if ( !g_pMaterialSystem )
		return false;

	//KeyValues* keyValues = new KeyValues( "Refract" );
	
	//m_pThermMaterial = g_pMaterialSystem->CreateMaterial( "thermoptic", keyValues );
	m_pThermMaterial = materials->FindMaterial("models/player/thermoptic", TEXTURE_GROUP_MODEL);
	m_pThermMaterial->AddRef();

	if ( !m_pThermMaterial )
		return false;

	bool found = false;

	m_pThermMaterial->SetShader( "Refract" );
	m_pThermMaterial->SetMaterialVarFlag( MATERIAL_VAR_MODEL, true );
	m_pThermMaterial->SetMaterialVarFlag( MATERIAL_VAR_SUPPRESS_DECALS, true );

	IMaterialVar* refractamount = m_pThermMaterial->FindVar( "$refractamount", &found );
	if ( found )
		refractamount->SetFloatValue( 0.8f );

	IMaterialVar* normalmap = m_pThermMaterial->FindVar( "$normalmap", &found );
	if ( found )
	{
		ITexture* waterTexture = g_pMaterialSystem->FindTexture( "dev/water_normal", "ClientEffect textures" );
		if ( waterTexture )
			normalmap->SetTextureValue( waterTexture );
	}
	
	IMaterialVar* dudvmap = m_pThermMaterial->FindVar( "$dudvmap", &found );
	if (found)
	{
		ITexture* waterTexture = g_pMaterialSystem->FindTexture( "dev/water_dudv", "ClientEffect textures" );
		if ( waterTexture )
			dudvmap->SetTextureValue( waterTexture );
	}

	IMaterialVar* bumpframe = m_pThermMaterial->FindVar( "$bumpframe", &found );
	if ( found )
		bumpframe->SetIntValue( 0 );	 

	return true;
}  		