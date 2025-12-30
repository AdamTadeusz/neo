#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_cartoonshader_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_cs_colour_posterise_steps("mat_neo_cs_colour_posterise_steps", "20", FCVAR_ARCHIVE, "variable in the colour palette size reduction step");

ConVar mat_neo_cs_outline_close_depth_difference("mat_neo_cs_outline_close_depth_difference", "2", FCVAR_ARCHIVE, "how big of a depth difference results in an outline closer to the camera");
ConVar mat_neo_cs_outline_far_depth_difference("mat_neo_cs_outline_far_depth_difference", "2", FCVAR_ARCHIVE, "how big of a depth difference results in an outline further from the camera");
ConVar mat_neo_cs_outline_close_width("mat_neo_cs_outline_close_width", "3.0f", FCVAR_ARCHIVE, "Width of objects closer to the camera");
ConVar mat_neo_cs_outline_far_width("mat_neo_cs_outline_far_width", "1.0f", FCVAR_ARCHIVE, "Width of objects further from the camera");

BEGIN_SHADER_FLAGS(Neo_CartoonShader, "Cartoon Pixel Shader", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(DBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameDepth_Alt", "")
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
	
	if (params[DBTEXTURE]->IsDefined())
	{
		LoadTexture(DBTEXTURE);
	}
	else
	{
		Assert(false);
	}
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
		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, NULL, 0);
		pShaderShadow->EnableDepthWrites(false);

		DECLARE_STATIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_cartoonshader_ps20b);
		SET_STATIC_PIXEL_SHADER(neo_cartoonshader_ps20b);

		// On DX9, get the gamma read and write correct
		if (g_pHardwareConfig->SupportsSRGB())
		{
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1, true);
			pShaderShadow->EnableSRGBWrite(true);
		}
		else
		{
			Assert(false);
		}
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE);
		BindTexture(SHADER_SAMPLER1, DBTEXTURE);
		
		const float flColourPosterizeSteps = mat_neo_cs_colour_posterise_steps.GetFloat();
		const float colour[4] = {flColourPosterizeSteps, 0.0f /*unused*/, 0.0f /*unused*/, 0.0f /*unused*/};
		pShaderAPI->SetPixelShaderConstant(0, colour);

		const float flOutlineDepthDifference = mat_neo_cs_outline_close_depth_difference.GetFloat();
		const float flOutlineWidth = mat_neo_cs_outline_close_width.GetFloat();

		ITexture *pTarget = params[ DBTEXTURE ]->GetTextureValue();
		const float outline[4] = {flOutlineDepthDifference, flOutlineWidth, 1.f / pTarget->GetActualWidth(), 1.f / pTarget->GetActualHeight()};
		pShaderAPI->SetPixelShaderConstant(1, outline);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_cartoonshader_ps20b);
		SET_DYNAMIC_PIXEL_SHADER(neo_cartoonshader_ps20b);
	}

	Draw();
}
END_SHADER
