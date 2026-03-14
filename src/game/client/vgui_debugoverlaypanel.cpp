//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "idebugoverlaypanel.h"
#include "overlaytext.h"
#include <vgui/IVGui.h>
#include "engine/ivdebugoverlay.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/IScheme.h>
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CDebugOverlay : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
	CDebugOverlay( vgui::VPANEL parent );
	virtual ~CDebugOverlay( void );

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();
	virtual void OnTick( void );

	virtual bool ShouldDraw( void );

private:
	vgui::HFont			m_hFont;
};

//-----------------------------------------------------------------------------
// Purpose: Instances the overlay object
// Input  : *parent - 
//-----------------------------------------------------------------------------
CDebugOverlay::CDebugOverlay( vgui::VPANEL parent ) :
	BaseClass( NULL, "CDebugOverlay" )
{
	int w, h;
	vgui::surface()->GetScreenSize( w, h );
	SetParent( parent );
	SetSize( w, h );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );

	m_hFont = 0;
	SetFgColor( Color( 0, 0, 0, 0 ) );
	SetPaintBackgroundEnabled( false );

	// set the scheme before any child control is created
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS ), "resource/ClientScheme.res", "Client");
	SetScheme( scheme );
	
	vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDebugOverlay::~CDebugOverlay( void )
{
}

void CDebugOverlay::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Use a large font
//	m_hFont = pScheme->GetFont( "Default" );
	m_hFont = pScheme->GetFont( "DebugOverlay" );
	assert( m_hFont );

	int w, h;
	vgui::surface()->GetScreenSize( w, h );
	SetSize( w, h );
	SetPos( 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDebugOverlay::OnTick( void )
{
	bool bVisible = ShouldDraw();
	if ( IsVisible() != bVisible )
	{
		SetVisible( bVisible );
	}
}

bool CDebugOverlay::ShouldDraw( void )
{
	if ( debugoverlay && debugoverlay->GetFirst() )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Paints the 2D overlay items to the screen
//-----------------------------------------------------------------------------
void CDebugOverlay::Paint()
{
	if (!debugoverlay)
		return;

	OverlayText_t* pCurrText = debugoverlay->GetFirst();
	while (pCurrText) 
	{
#ifdef NEO
		if (*pCurrText->text)
#else
		if ( pCurrText->text != NULL ) 
#endif
		{
			// --------------
			// Draw the text
			// --------------
			int r = pCurrText->r;
			int g = pCurrText->g;
			int b = pCurrText->b;
			int a = pCurrText->a;
			Vector screenPos;

			if (pCurrText->bUseOrigin)
			{
				if (!debugoverlay->ScreenPosition( pCurrText->origin, screenPos )) 
				{
					float xPos		= screenPos[0];
					float yPos		= screenPos[1]+ (pCurrText->lineOffset*13); // Line spacing;
					g_pMatSystemSurface->DrawColoredText( m_hFont, xPos, yPos, r, g, b, a, "%s", pCurrText->text );
				}
			}
			else
			{
				if (!debugoverlay->ScreenPosition( pCurrText->flXPos,pCurrText->flYPos, screenPos )) 
				{	
					float xPos		= screenPos[0];
					float yPos		= screenPos[1]+ (pCurrText->lineOffset*13); // Line spacing;
					g_pMatSystemSurface->DrawColoredText( m_hFont, xPos, yPos, r, g, b, a, "%s", pCurrText->text );
				}
			}
		}
		pCurrText = debugoverlay->GetNext( pCurrText );
	}

	debugoverlay->ClearDeadOverlays();
}

class CDebugOverlayPanel : public IDebugOverlayPanel
{
private:
	CDebugOverlay *debugOverlayPanel;
public:
	CDebugOverlayPanel( void )
	{
		debugOverlayPanel = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		debugOverlayPanel = new CDebugOverlay( parent );
	}
	void Destroy( void )
	{
		if ( debugOverlayPanel )
		{
			debugOverlayPanel->SetParent( (vgui::Panel *)NULL );
			debugOverlayPanel->MarkForDeletion();
			debugOverlayPanel = NULL;
		}
	}
};

static CDebugOverlayPanel g_DebugOverlay;
IDebugOverlayPanel *debugoverlaypanel =  ( IDebugOverlayPanel * )&g_DebugOverlay;


void DebugDrawLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, int r, int g, int b, bool test, float duration )
{
	if ( debugoverlay )
	{
		debugoverlay->AddLineOverlay( vecAbsStart + Vector( 0,0,0.1), vecAbsEnd + Vector( 0,0,0.1), r,g,b, test, duration );
	}
}

// Appropriated from NDebugOverlay::Circle
void DebugDrawCollisionCircle( const Vector &position, const Vector &xAxis, const Vector &yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
{
	if (!debugoverlay)
		return;

	const unsigned int nSegments = 72;
	const float flRadStep = (M_PI*2.0f) / (float) nSegments;

	Vector vecLastPosition;
	
	// Find our first position
	Vector vecStart = position + xAxis * radius;
	trace_t	tr;
	UTIL_TraceLine( position, vecStart, MASK_SOLID, nullptr, COLLISION_GROUP_NONE, &tr );
	if (tr.fraction != 1.0)
	{
		vecStart = position + ((-position + vecStart) * tr.fraction);
	}
	Vector vecPosition = vecStart;

	// Draw out each segment
	for ( int i = 1; i <= nSegments; i++ )
	{
		// Store off our last position
		vecLastPosition = vecPosition;

		// Calculate the new one
		float flSin, flCos;
		SinCos( flRadStep*i, &flSin, &flCos );
		vecPosition = position + (xAxis * flCos * radius) + (yAxis * flSin * radius);
		UTIL_TraceLine( position, vecPosition, MASK_SOLID, nullptr, COLLISION_GROUP_NONE, &tr );
		if (tr.fraction != 1.0)
		{
			vecPosition = position + ((-position + vecPosition) * tr.fraction);
		}

		const Vector direction = (vecPosition - vecLastPosition).Normalized();
		const Vector positionDirection = (position - vecLastPosition).Normalized();
		
		if ((abs(DotProduct(direction, yAxis)) > 0.99 && abs(DotProduct(positionDirection, direction)) > 0.99) || (vecPosition.DistToSqr(position) < (32 * 32) && vecLastPosition.DistToSqr(position) < (32 * 32)))
		{
			continue;
		}

		// Draw the line
		debugoverlay->AddLineOverlay( vecLastPosition, vecPosition, r, g, b, bNoDepthTest, flDuration + (i * 0.01) );
	}
}