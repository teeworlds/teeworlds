#ifndef MODAPI_ENTITY_H
#define MODAPI_ENTITY_H

#include <game/server/entity.h>

#include <base/tl/array.h>

class CModAPI_EntitySnapshot07 : public CEntity
{
private:
	int m_NbIdPerSnapshot;
	array<int> m_IDList;
	
public:
	CModAPI_EntitySnapshot07(CGameWorld *pGameWorld, int Objtype, vec2 Pos, int ProximityRadius=0, int NbIdPerSnapshot=1);
	~CModAPI_EntitySnapshot07();
		
	virtual void Snap07(int SnappingClient);
	virtual void Snap07ModAPI(int SnappingClient);

protected:
	virtual void Snap(int SnappingClient, int FirstID);
	
	int GetID(int ID);
};

#endif
