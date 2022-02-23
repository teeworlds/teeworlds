#include "water.h"

#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/client/components/maplayers.h>
#include <game/collision.h>
#include <generated/client_data.h>
#include <engine/config.h>
#include <game/client/gameclient.h>
#include <game/client/components/effects.h>
#include <engine/shared/config.h>

enum {
	DESIRED_HEIGHT = 16
};

void CWater::Init()
{
	m_pLayers = Layers();
	if (!m_pLayers->WaterLayer())
		return;

	m_WaveVar = 1.0f;

	int NumOfSurfaces = 0;
	int Length;
	m_pWaterTiles = static_cast<CTile*>(m_pLayers->Map()->GetData(m_pLayers->WaterLayer()->m_Data));
	int m_Width = m_pLayers->WaterLayer()->m_Width;
	int m_Height = m_pLayers->WaterLayer()->m_Height;
	m_Color.a = m_pLayers->WaterLayer()->m_Color.a / 255.0f;
	m_Color.b = m_pLayers->WaterLayer()->m_Color.b / 255.0f;
	m_Color.r = m_pLayers->WaterLayer()->m_Color.r / 255.0f;
	m_Color.g = m_pLayers->WaterLayer()->m_Color.g / 255.0f;

	//Algorithm to fetch the amount of 'Surface' Areas
	for (int i = m_Width; i < m_Width * m_Height; i++)
	{
		if (m_pWaterTiles[i].m_Index==1 && m_pWaterTiles[i-m_Width].m_Index!=1)
		{
			Length = AmountOfSurfaceTiles(i, m_Width);
			NumOfSurfaces++;
			i += Length;
		}
	}
	m_NumOfSurfaces = NumOfSurfaces;
	m_aWaterSurfaces = new CWaterSurface * [NumOfSurfaces];
	NumOfSurfaces = 0;
	for (int i = m_Width; i < m_Width * m_Height; i++)
	{
		if (m_pWaterTiles[i].m_Index == 1 && m_pWaterTiles[i - m_Width].m_Index != 1)
		{
			Length = AmountOfSurfaceTiles(i, m_Width);
			m_aWaterSurfaces[NumOfSurfaces] = new CWaterSurface(i % m_Width, i / m_Width, Length);
			NumOfSurfaces++;
			i += Length;
		}
	}
}

void CWater::OnReset()
{
	if (Client()->State() == IClient::STATE_OFFLINE || !Config()->m_GfxAnimateWater)
	{
		m_pLayers = 0x0;
		m_pWaterTiles = 0x0;
		for (int i = 0; i < m_NumOfSurfaces; i++)
		{
			m_aWaterSurfaces[i]->Remove();
		}
		m_NumOfSurfaces = 0;
		m_WaveVar = 0;

		delete[] m_aWaterSurfaces;
		m_aWaterSurfaces = 0;
	}
}

int CWater::AmountOfSurfaceTiles(int Coord, int End)
{
	if (!((m_pWaterTiles[Coord].m_Index == 1) && !(m_pWaterTiles[Coord - End].m_Index == 1)))
		return 0;

	//Check if we are not at the end of the x coordinate
	if (!((Coord+1) % End))
		return 1;

	//If we are not, we should check another tile
	else
		return (1 + AmountOfSurfaceTiles(Coord + 1, End));
}

void CWater::Render()
{
	m_pLayers = Layers();
	if (!m_pLayers->WaterLayer())
		return;
	if (!Config()->m_GfxAnimateWater)
		return;
	int64 Now = time_get();
	static int64 s_LastTime = Now;   //TODO, IMPLEMENT LATER
	Tick((float)((Now - s_LastTime) / (double)time_freq()) * m_pClient->GetAnimationPlaybackSpeed());
	s_LastTime = Now;
	//Tick();
	
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_WATER].m_Id);
	Graphics()->QuadsBegin();

	int SurfaceCount = m_NumOfSurfaces ? m_NumOfSurfaces : 0;
	vec4 RenderWaterColor = WaterColor();
	for (int i = 0;i < SurfaceCount;i++)
	{
		float XBasePos = m_aWaterSurfaces[i]->m_Coordinates.m_X * 32.0f;
		float YBasePos = (float)m_aWaterSurfaces[i]->m_Coordinates.m_Y * 32.0f;
		RenderTools()->SelectSprite(SPRITE_WATER30);
		for (int j = 0; j < m_aWaterSurfaces[i]->m_AmountOfVertex - 1; j++)
		{
			for (int k = j + 1; k < m_aWaterSurfaces[i]->m_AmountOfVertex - 1; k++)
			{
				if (m_aWaterSurfaces[i]->m_aVertex[j]->m_Height == m_aWaterSurfaces[i]->m_aVertex[k]->m_Height && k + 1 != m_aWaterSurfaces[i]->m_AmountOfVertex - 1)
				{
					continue;
				}
				else
				{
					if (!(j == k - 1))
					{
						Graphics()->SetColor(RenderWaterColor.r, RenderWaterColor.g, RenderWaterColor.b, RenderWaterColor.a);
						WaterFreeform(XBasePos, YBasePos, j, k - 1, 4, m_aWaterSurfaces[i]);
						if (Config()->m_GfxWaterOutline)
						{
							Graphics()->SetColor((RenderWaterColor.r + 1.0f) / 2, (RenderWaterColor.g + 1.0f) / 2, (RenderWaterColor.b + 1.0f) / 2, RenderWaterColor.a);
							WaterFreeformOutline(XBasePos, YBasePos, j, k - 1, 4, m_aWaterSurfaces[i]);
						}
					}
					j += k - 1 - j;
					break;
				}
			}
			Graphics()->SetColor(RenderWaterColor.r, RenderWaterColor.g, RenderWaterColor.b, RenderWaterColor.a);
			WaterFreeform(XBasePos, YBasePos, j + 1, j, 4, m_aWaterSurfaces[i]);
			if (Config()->m_GfxWaterOutline)
			{
				Graphics()->SetColor((RenderWaterColor.r + 1.0f) / 2, (RenderWaterColor.g + 1.0f) / 2, (RenderWaterColor.b + 1.0f) / 2, RenderWaterColor.a);
				WaterFreeformOutline(XBasePos, YBasePos, j + 1, j, 4, m_aWaterSurfaces[i]);
			}
		}
		//WaterFreeform(XBasePos, YBasePos, m_aWaterSurfaces[i]->m_AmountOfVertex - 1, m_aWaterSurfaces[i]->m_AmountOfVertex -2, 4, m_aWaterSurfaces[i]);
	}
	Graphics()->QuadsEnd();
	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}
void CWater::WaterFreeform(float X, float Y, int A, int B, float Size, CWaterSurface* Surface)
{
	IGraphics::CFreeformItem Item(
		X + A * Size, //bottom left corner
		Y + 32.0f,
		X + B * Size, //bottom right corner
		Y + 32.0f, 
		X + A * 4, //top left corner
		Y + 32.0f - 16.0f - (Surface->m_aVertex[A]->m_Height - 16.0f) * Surface->m_Scale,
		X + B * 4, //top right corner
		Y + 32.0f - 16.0f - (Surface->m_aVertex[B]->m_Height - 16.0f) * Surface->m_Scale

	);
	Graphics()->QuadsDrawFreeform(&Item, 1);
	
}

void CWater::WaterFreeformOutline(float X, float Y, int A, int B, float Size, CWaterSurface* Surface)
{
	IGraphics::CFreeformItem Item(
		X + A * 4, //bottom left corner
		Y + 32.0f - 16.0f - (Surface->m_aVertex[A]->m_Height - 16.0f) * Surface->m_Scale,
		X + B * 4, //bottom right corner
		Y + 32.0f - 16.0f - (Surface->m_aVertex[B]->m_Height - 16.0f) * Surface->m_Scale,
		X + A * 4, //top left corner
		Y + 32.0f - 16.0f - (Surface->m_aVertex[A]->m_Height - 16.0f) * Surface->m_Scale - 3.0f,
		X + B * 4, //top right corner
		Y + 32.0f - 16.0f - (Surface->m_aVertex[B]->m_Height - 16.0f) * Surface->m_Scale - 3.0f

	);
	Graphics()->QuadsDrawFreeform(&Item, 1);
}

bool CWater::HitWater(float x, float y, float Force)
{
	//Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "HitWater");
	CWaterSurface* Target;
	if(FindSurface(&Target, x, y))
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "HitWater2");
		if (Target->HitWater(x, y, Force))
		{
			if (absolute(Force) >= 5.0f)
				m_pClient->m_pEffects->Droplet(vec2(x, y), vec2(0, Force));
			return true;
		}
	}
	return false;
}

void CWater::CreateWave(float x, float y, float Force)
{
	CWaterSurface* Target;
	if(FindSurface(&Target, x, y))
	{
		Target->CreateWave(x, y, Force, Config()->m_GfxWaveDivider2, Config()->m_GfxWavePushDivider);
	}
}

bool CWater::FindSurface(CWaterSurface** Pointer, float x, float y)
{
	for (int i = 0; i < m_NumOfSurfaces;i++)
	{
		if (y < m_aWaterSurfaces[i]->m_Coordinates.m_Y * 32.0f + 32.0f)
		{
			if (y > m_aWaterSurfaces[i]->m_Coordinates.m_Y * 32.0f)
			{
				if (x > m_aWaterSurfaces[i]->m_Coordinates.m_X * 32.0f && x < (m_aWaterSurfaces[i]->m_Coordinates.m_X + m_aWaterSurfaces[i]->m_Length) * 32.0f)
				{
					*Pointer = m_aWaterSurfaces[i];
					
					return true;
				}
				else
					continue;
			}
			else
				return false; //the explanation is a bit complicated
		}
		else
			continue;
	}
	return false;
}

bool CWater::IsUnderWater(vec2 Pos)
{
	CWaterSurface* Target;
	if (FindSurface(&Target, Pos.x, Pos.y))
	{
		return Target->IsUnderWater(Pos);
	}
	else
	{
		return m_pClient->Collision()->TestBox(Pos, vec2(1.0f, 1.0f), 8);
	}
}

void CWater::Tick(float TimePassed)
{
	if (!(Config()->m_GfxAnimateWater))
	{
		OnReset();
		return;
	}
	
	//Calculate the amount of 'Ticks'
	m_WaveVar += TimePassed;
	float Inverse = 1.0f / Config()->m_GfxNumOfWaveUpdates;
	int Frames = m_WaveVar / Inverse;
	m_WaveVar -= Frames * Inverse;
	m_WaveVar = clamp(m_WaveVar, 0.0f, 1.0f);
	
	for (int i = 0; i < m_NumOfSurfaces;i++)
		for (int j = 0; j < Frames; j++)
			m_aWaterSurfaces[i]->Tick(TimePassed);
}

vec4 CWater::WaterColor()
{
	float r = 1, g = 1, b = 1, a = 1;

	if (m_pLayers->WaterLayer()->m_ColorEnv >= 0)
	{
		float aChannels[4];
		ENVELOPE_EVAL pfnEval = m_pClient->m_pMapLayersForeGround->EnvelopeEval;
		pfnEval(m_pLayers->WaterLayer()->m_ColorEnvOffset / 1000.0f, m_pLayers->WaterLayer()->m_ColorEnv, aChannels, m_pClient->m_pMapLayersForeGround);
		r = aChannels[0];
		g = aChannels[1];
		b = aChannels[2];
		a = aChannels[3];
	}
	const float Alpha = m_Color.a * a;

	return vec4(m_Color.r * r * Alpha, m_Color.g * g * Alpha, m_Color.b * b * Alpha, Alpha);
}

CWaterSurface::CWaterSurface(int X, int Y, int Length)
{
	m_Coordinates.m_X = X;
	m_Coordinates.m_Y = Y;
	m_Length = Length;
	m_AmountOfVertex = Length * 8 + 1;
	m_aVertex = new CVertex* [m_AmountOfVertex];
	m_Scale = 1.0f;

	for (int i = 0; i < m_AmountOfVertex; i++)
	{
		m_aVertex[i] = new CVertex(16.0f);
	}
}

bool CWaterSurface::HitWater(float x, float y, float Force)
{
	//Do some magic to find the correct Vertex
	int RealX = (int)(x - m_Coordinates.m_X * 32.0f);
	RealX /= 4;

	//Let the wave cool off a bit
	if (m_aVertex[RealX]->m_RecentWave>0)
		return false;

	m_aVertex[RealX]->m_Velo += Force;

	//Set the cooldown
	m_aVertex[RealX]->m_RecentWave += 0.5;
	for (int i = 0; i < 16; i++)
	{
		if(RealX - i >= 0)
			m_aVertex[RealX - i]->m_RecentWave += 0.5;
		if (RealX + i < m_AmountOfVertex)
			m_aVertex[RealX + i]->m_RecentWave += 0.5;
	}

	return true;
}

void CWaterSurface::CreateWave(float x, float y, float Force, int WaveStrDivider, int WavePushDivider)
{
	float PositionForce = absolute(Force);

	//Do some magic to find the correct Vertex
	int RealX = (int)(x - m_Coordinates.m_X * 32.0f);
	RealX /= 4;

	//Let the wave cool off a bit
	//if (m_aVertex[RealX]->m_RecentWave > 0)
		//return;

	int Side = Force>0 ? 1 : -1;

	Force = absolute(Force) / (WaveStrDivider * 1.0f);

	m_aVertex[RealX]->m_Velo -= Force;
	for (int i = 1; i <= 4; i++)
	{
		if (RealX - i < 0 || RealX + i >= m_AmountOfVertex - 1)
			break;

		float AddedForce = Force * pow(0.5f, i);
		m_aVertex[RealX + i]->m_Velo -= AddedForce;
		m_aVertex[RealX - i]->m_Velo -= AddedForce;
	}

	RealX += Side * (PositionForce / WavePushDivider); //Predicting the wave
	RealX = clamp(RealX, 0, m_AmountOfVertex - 1);
	m_aVertex[RealX]->m_Velo += Force;
	for (int i = 1; i <= 4; i++)
	{
		if (RealX - i < 0 || RealX + i >= m_AmountOfVertex -1)
			break;

		float AddedForce = Force * pow(0.5f, i);
		m_aVertex[RealX + i]->m_Velo += AddedForce;
		m_aVertex[RealX - i]->m_Velo += AddedForce;
	}

	return;
}

float CWaterSurface::PositionOfVertex(float Height, bool ToScale)
{
	float Scale = ToScale ? m_Scale : 1.0f;
	return m_Coordinates.m_Y * 32.0f + 32.0f - 16.0f - (Height - 16.0f) * Scale;
}

bool CWaterSurface::IsUnderWater(vec2 Pos)
{
	//Find the Position in the array
	int RealX = (int)(Pos.x - m_Coordinates.m_X * 32.0f);
	RealX /= 4;

	if (PositionOfVertex(m_aVertex[RealX]->m_Height) < Pos.y)
	{
		if (RealX + 1 == m_AmountOfVertex || PositionOfVertex(m_aVertex[RealX + 1]->m_Height) < Pos.y)
			return true;
	}
	return false;
}

void CWaterSurface::Tick(float TimePassed)
{
	for (int i = 0; i < m_AmountOfVertex; i++)
	{
		if (m_aVertex[i]->m_RecentWave > 0)
			m_aVertex[i]->m_RecentWave -= TimePassed;;
		m_aVertex[i]->m_Height += m_aVertex[i]->m_Velo;
		m_aVertex[i]->m_Velo += ((m_aVertex[i]->m_Height - DESIRED_HEIGHT * 1.0f) * -0.005f - 0.005f * m_aVertex[i]->m_Velo);
	}

	float Spread = 0.1f;

	//makes the waves spread
	float* LeftDelta = new float[m_AmountOfVertex];
	float* RightDelta = new float[m_AmountOfVertex];

	for (int j = 0; j < 8; j++)
	{
		for (int i = 0; i < m_AmountOfVertex; i++)
		{
			if (i > 0)
			{
				LeftDelta[i] = Spread * (m_aVertex[i]->m_Height - m_aVertex[i - 1]->m_Height);
				m_aVertex[i - 1]->m_Velo += LeftDelta[i];
			}
			if (i < m_AmountOfVertex - 1)
			{
				RightDelta[i] = Spread * (m_aVertex[i]->m_Height - m_aVertex[i + 1]->m_Height);
				m_aVertex[i + 1]->m_Velo += RightDelta[i];
			}
		}
		for (int i = 0; i < m_AmountOfVertex; i++)
		{
			if (i > 0)
				m_aVertex[i - 1]->m_Height += LeftDelta[i];
			if (i < m_AmountOfVertex - 1)
				m_aVertex[i + 1]->m_Height += RightDelta[i];

		}
	}
	float MaxHeight = 0;
	float Scale = 1;
	for (int i = 0; i < m_AmountOfVertex; i++)
	{
		if (absolute(m_aVertex[i]->m_Height - 16) > MaxHeight)
			MaxHeight = absolute(m_aVertex[i]->m_Height - 16.0f);
	}
	if (MaxHeight > 16.0f)
	{
		Scale = 16.0f / MaxHeight;
	}
	m_Scale = Scale;
}

CVertex::CVertex(float Height)
{
	m_Height = Height;
	m_Velo = 0;
	m_RecentWave = 0;
}

void CWaterSurface::Remove()
{
	delete[] m_aVertex;
}
