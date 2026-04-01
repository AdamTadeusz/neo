#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_thermalvision_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SHADER_FLAGS( Neo_ThermalVision, "Help for the thermalvision shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(UTILTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_NEOVision", "")
SHADER_PARAM(MODIFYCOLOUR, SHADER_PARAM_TYPE_BOOL, "0", "")
SHADER_PARAM(STRENGTH, SHADER_PARAM_TYPE_FLOAT, "1", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[FBTEXTURE]->IsDefined())
	{
		LoadTexture(FBTEXTURE);
	}
	else
	{
		Assert(false);
	}

	if (params[UTILTEXTURE]->IsDefined())
	{
		LoadTexture(UTILTEXTURE);
	}
	else
	{
		Assert(false);
	}
}

bool NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame /* = true */) const
{
	return true;
}

SHADER_FALLBACK
{
	// Requires DX9 + above
	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Assert(0);
		return "Wireframe";
	}
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, false);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1, false);
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableDepthTest(false);

		int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);

		DECLARE_STATIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_thermalvision_ps30);
		SET_STATIC_PIXEL_SHADER(neo_thermalvision_ps30);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE);
		BindTexture(SHADER_SAMPLER1, UTILTEXTURE);
		
		ITexture *src_texture=params[FBTEXTURE]->GetTextureValue();
		float vPixelSizeModifyColourStrength[4] = {src_texture->GetActualWidth(), 
													src_texture->GetActualHeight(),
													params[MODIFYCOLOUR]->GetFloatValue(),
													params[STRENGTH]->GetFloatValue()};
		s_pShaderAPI->SetPixelShaderConstant(0, vPixelSizeModifyColourStrength);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_thermalvision_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_thermalvision_ps30);
	}

	Draw();
}

END_SHADER