#ifndef GAME_CLIENT_COMPONENTS_WATER_H
#define GAME_CLIENT_COMPONENTS_WATER_H
#include <game/client/component.h>
/*
	Class: Water
		The purpose of this component is to handle waves and the generation process
		As such, the component only renders the top layer of the water.
*/
class CWater : public CComponent
{
public:
	CWater() { };
	
	class CLayers* m_pLayers;
	class CWaterSurface** m_aWaterSurfaces;
	struct CTile* m_pWaterTiles;
	
	virtual void OnMapLoad() { Init(); }
	virtual void OnReset();

	void Render();
	void Init();
	void Tick(float TimePassed);

	int AmountOfSurfaceTiles(int Coord, int End);
	bool HitWater(float x, float y, float Force);
	void CreateWave(float x, float y, float Force);
	void WaterFreeform(float X, float Y, int A, int B, float Size, CWaterSurface* Surface);
	void WaterFreeformOutline(float X, float Y, int A, int B, float Size, CWaterSurface* Surface);
	bool IsUnderWater(vec2 Pos);
	bool FindSurface(CWaterSurface** Pointer, float x, float y);
	vec4 WaterColor();

	vec4 m_Color;
	int m_NumOfSurfaces;
private:
	float m_WaveVar;
};

/*
	Class: CWaterSurface
		Contains a line of CVertex, which, together, make a continuos wave.
*/
class CWaterSurface
{
public:
	CWaterSurface(int X, int Y, int Length);

	struct Coordinates
	{
		int m_X;
		int m_Y;
	} m_Coordinates;

	class CVertex** m_aVertex;
	int m_AmountOfVertex;
	int m_Length;
	float m_Scale;
	
	bool IsUnderWater(vec2 Pos);
	void Remove();
	void Tick(float TimePassed);
	bool HitWater(float x, float y, float Force);
	void CreateWave(float x, float y, float Force, int WaveStrDivider, int WavePushDivider); //Created while moving horizontally
	float PositionOfVertex(float Height, bool ToScale = true);
};

/*
	Class: CVertex
		Contains information about a vertice's velocity and Height
*/
class CVertex
{
public:
	CVertex(float Height);
	float m_RecentWave;
	float m_Velo;
	float m_Height;
};
#endif