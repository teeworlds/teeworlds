/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>
#include <base/math.h>
#include <engine/graphics.h>

#include "render.h"

void CRenderTools::RenderEvalEnvelope(CEnvPoint *pPoints, int NumPoints, int Channels, float Time, float *pResult)
{
	if(NumPoints == 0)
	{
		pResult[0] = 0;
		pResult[1] = 0;
		pResult[2] = 0;
		pResult[3] = 0;
		return;
	}

	if(NumPoints == 1)
	{
		pResult[0] = fx2f(pPoints[0].m_aValues[0]);
		pResult[1] = fx2f(pPoints[0].m_aValues[1]);
		pResult[2] = fx2f(pPoints[0].m_aValues[2]);
		pResult[3] = fx2f(pPoints[0].m_aValues[3]);
		return;
	}

	Time = fmod(Time, pPoints[NumPoints-1].m_Time/1000.0f)*1000.0f;
	for(int i = 0; i < NumPoints-1; i++)
	{
		if(Time >= pPoints[i].m_Time && Time <= pPoints[i+1].m_Time)
		{
			float Delta = pPoints[i+1].m_Time-pPoints[i].m_Time;
			float a = (Time-pPoints[i].m_Time)/Delta;


			if(pPoints[i].m_Curvetype == CURVETYPE_SMOOTH)
				a = -2*a*a*a + 3*a*a; // second hermite basis
			else if(pPoints[i].m_Curvetype == CURVETYPE_SLOW)
				a = a*a*a;
			else if(pPoints[i].m_Curvetype == CURVETYPE_FAST)
			{
				a = 1-a;
				a = 1-a*a*a;
			}
			else if (pPoints[i].m_Curvetype == CURVETYPE_STEP)
				a = 0;
			else
			{
				// linear
			}

			for(int c = 0; c < Channels; c++)
			{
				float v0 = fx2f(pPoints[i].m_aValues[c]);
				float v1 = fx2f(pPoints[i+1].m_aValues[c]);
				pResult[c] = v0 + (v1-v0) * a;
			}

			return;
		}
	}

	pResult[0] = fx2f(pPoints[NumPoints-1].m_aValues[0]);
	pResult[1] = fx2f(pPoints[NumPoints-1].m_aValues[1]);
	pResult[2] = fx2f(pPoints[NumPoints-1].m_aValues[2]);
	pResult[3] = fx2f(pPoints[NumPoints-1].m_aValues[3]);
	return;
}


static void Rotate(CPoint *pCenter, CPoint *pPoint, float Rotation)
{
	int x = pPoint->x - pCenter->x;
	int y = pPoint->y - pCenter->y;
	pPoint->x = (int)(x * cosf(Rotation) - y * sinf(Rotation) + pCenter->x);
	pPoint->y = (int)(x * sinf(Rotation) + y * cosf(Rotation) + pCenter->y);
}

void CRenderTools::RenderQuads(CQuad *pQuads, int NumQuads, int RenderFlags, ENVELOPE_EVAL pfnEval, void *pUser)
{
	Graphics()->QuadsBegin();
	float Conv = 1/255.0f;
	for(int i = 0; i < NumQuads; i++)
	{
		CQuad *q = &pQuads[i];

		float r=1, g=1, b=1, a=1;

		if(q->m_ColorEnv >= 0)
		{
			float aChannels[4];
			pfnEval(q->m_ColorEnvOffset/1000.0f, q->m_ColorEnv, aChannels, pUser);
			r = aChannels[0];
			g = aChannels[1];
			b = aChannels[2];
			a = aChannels[3];
		}

		bool Opaque = false;
		/* TODO: Analyze quadtexture
		if(a < 0.01f || (q->m_aColors[0].a < 0.01f && q->m_aColors[1].a < 0.01f && q->m_aColors[2].a < 0.01f && q->m_aColors[3].a < 0.01f))
			Opaque = true;
		*/
		if(Opaque && !(RenderFlags&LAYERRENDERFLAG_OPAQUE))
			continue;
		if(!Opaque && !(RenderFlags&LAYERRENDERFLAG_TRANSPARENT))
			continue;

		Graphics()->QuadsSetSubsetFree(
			fx2f(q->m_aTexcoords[0].x), fx2f(q->m_aTexcoords[0].y),
			fx2f(q->m_aTexcoords[1].x), fx2f(q->m_aTexcoords[1].y),
			fx2f(q->m_aTexcoords[2].x), fx2f(q->m_aTexcoords[2].y),
			fx2f(q->m_aTexcoords[3].x), fx2f(q->m_aTexcoords[3].y)
		);

		float OffsetX = 0;
		float OffsetY = 0;
		float Rot = 0;

		// TODO: fix this
		if(q->m_PosEnv >= 0)
		{
			float aChannels[4];
			pfnEval(q->m_PosEnvOffset/1000.0f, q->m_PosEnv, aChannels, pUser);
			OffsetX = aChannels[0];
			OffsetY = aChannels[1];
			Rot = aChannels[2]/360.0f*pi*2;
		}

		IGraphics::CColorVertex Array[4] = {
			IGraphics::CColorVertex(0, q->m_aColors[0].r*Conv*r, q->m_aColors[0].g*Conv*g, q->m_aColors[0].b*Conv*b, q->m_aColors[0].a*Conv*a),
			IGraphics::CColorVertex(1, q->m_aColors[1].r*Conv*r, q->m_aColors[1].g*Conv*g, q->m_aColors[1].b*Conv*b, q->m_aColors[1].a*Conv*a),
			IGraphics::CColorVertex(2, q->m_aColors[2].r*Conv*r, q->m_aColors[2].g*Conv*g, q->m_aColors[2].b*Conv*b, q->m_aColors[2].a*Conv*a),
			IGraphics::CColorVertex(3, q->m_aColors[3].r*Conv*r, q->m_aColors[3].g*Conv*g, q->m_aColors[3].b*Conv*b, q->m_aColors[3].a*Conv*a)};
		Graphics()->SetColorVertex(Array, 4);

		CPoint *pPoints = q->m_aPoints;

		if(Rot != 0)
		{
			static CPoint aRotated[4];
			aRotated[0] = q->m_aPoints[0];
			aRotated[1] = q->m_aPoints[1];
			aRotated[2] = q->m_aPoints[2];
			aRotated[3] = q->m_aPoints[3];
			pPoints = aRotated;

			Rotate(&q->m_aPoints[4], &aRotated[0], Rot);
			Rotate(&q->m_aPoints[4], &aRotated[1], Rot);
			Rotate(&q->m_aPoints[4], &aRotated[2], Rot);
			Rotate(&q->m_aPoints[4], &aRotated[3], Rot);
		}

		IGraphics::CFreeformItem Freeform(
			fx2f(pPoints[0].x)+OffsetX, fx2f(pPoints[0].y)+OffsetY,
			fx2f(pPoints[1].x)+OffsetX, fx2f(pPoints[1].y)+OffsetY,
			fx2f(pPoints[2].x)+OffsetX, fx2f(pPoints[2].y)+OffsetY,
			fx2f(pPoints[3].x)+OffsetX, fx2f(pPoints[3].y)+OffsetY);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	}
	Graphics()->QuadsEnd();
}

void CRenderTools::RenderTilemap(CTile *pTiles, int w, int h, float Scale, vec4 Color, int RenderFlags,
									ENVELOPE_EVAL pfnEval, void *pUser, int ColorEnv, int ColorEnvOffset)
{
	//Graphics()->TextureSet(img_get(tmap->image));
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	//Graphics()->MapScreen(screen_x0-50, screen_y0-50, screen_x1+50, screen_y1+50);

	// calculate the final pixelsize for the tiles
	float TilePixelSize = 1024/32.0f;
	float FinalTileSize = Scale/(ScreenX1-ScreenX0) * Graphics()->ScreenWidth();
	float FinalTilesetScale = FinalTileSize/TilePixelSize;

	float r=1, g=1, b=1, a=1;
	if(ColorEnv >= 0)
	{
		float aChannels[4];
		pfnEval(ColorEnvOffset/1000.0f, ColorEnv, aChannels, pUser);
		r = aChannels[0];
		g = aChannels[1];
		b = aChannels[2];
		a = aChannels[3];
	}

	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r*r, Color.g*g, Color.b*b, Color.a*a);

	int StartY = (int)(ScreenY0/Scale)-1;
	int StartX = (int)(ScreenX0/Scale)-1;
	int EndY = (int)(ScreenY1/Scale)+1;
	int EndX = (int)(ScreenX1/Scale)+1;

	// adjust the texture shift according to mipmap level
	float TexSize = 1024.0f;
	float Frac = (1.25f/TexSize) * (1/FinalTilesetScale);
	float Nudge = (0.5f/TexSize) * (1/FinalTilesetScale);

	for(int y = StartY; y < EndY; y++)
		for(int x = StartX; x < EndX; x++)
		{
			int mx = x;
			int my = y;

			if(RenderFlags&TILERENDERFLAG_EXTEND)
			{
				if(mx<0)
					mx = 0;
				if(mx>=w)
					mx = w-1;
				if(my<0)
					my = 0;
				if(my>=h)
					my = h-1;
			}
			else
			{
				if(mx<0)
					continue; // mx = 0;
				if(mx>=w)
					continue; // mx = w-1;
				if(my<0)
					continue; // my = 0;
				if(my>=h)
					continue; // my = h-1;
			}

			int c = mx + my*w;

			unsigned char Index = pTiles[c].m_Index;
			if(Index)
			{
				unsigned char Flags = pTiles[c].m_Flags;

				bool Render = false;
				if(Flags&TILEFLAG_OPAQUE && Color.a*a > 254.0f/255.0f)
				{
					if(RenderFlags&LAYERRENDERFLAG_OPAQUE)
						Render = true;
				}
				else
				{
					if(RenderFlags&LAYERRENDERFLAG_TRANSPARENT)
						Render = true;
				}

				if(Render)
				{

					int tx = Index%16;
					int ty = Index/16;
					int Px0 = tx*(1024/16);
					int Py0 = ty*(1024/16);
					int Px1 = Px0+(1024/16)-1;
					int Py1 = Py0+(1024/16)-1;

					float x0 = Nudge + Px0/TexSize+Frac;
					float y0 = Nudge + Py0/TexSize+Frac;
					float x1 = Nudge + Px1/TexSize-Frac;
					float y1 = Nudge + Py0/TexSize+Frac;
					float x2 = Nudge + Px1/TexSize-Frac;
					float y2 = Nudge + Py1/TexSize-Frac;
					float x3 = Nudge + Px0/TexSize+Frac;
					float y3 = Nudge + Py1/TexSize-Frac;

					if(Flags&TILEFLAG_VFLIP)
					{
						x0 = x2;
						x1 = x3;
						x2 = x3;
						x3 = x0;
					}

					if(Flags&TILEFLAG_HFLIP)
					{
						y0 = y3;
						y2 = y1;
						y3 = y1;
						y1 = y0;
					}

					if(Flags&TILEFLAG_ROTATE)
					{
						float Tmp = x0;
						x0 = x3;
						x3 = x2;
						x2 = x1;
						x1 = Tmp;
						Tmp = y0;
						y0 = y3;
						y3 = y2;
						y2 = y1;
						y1 = Tmp;
 					}

					Graphics()->QuadsSetSubsetFree(x0, y0, x1, y1, x2, y2, x3, y3);
					IGraphics::CQuadItem QuadItem(x*Scale, y*Scale, Scale, Scale);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
				}
			}
			x += pTiles[c].m_Skip;
		}

	Graphics()->QuadsEnd();
	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}
