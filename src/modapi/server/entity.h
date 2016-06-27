#ifndef MODAPI_ENTITY_H
#define MODAPI_ENTITY_H

#include <game/server/entity.h>
#include <modapi/compatibility.h>

#include <base/tl/array.h>

class CModAPI_Entity : public CEntity
{
private:
	int m_NbIdPerSnapshot;
	array<int> m_IDList;
	
public:
	CModAPI_Entity(CGameWorld *pGameWorld, int Objtype, vec2 Pos, int ProximityRadius=0, int NbIdPerSnapshot=1);
	~CModAPI_Entity();
		
	virtual void Snap06(int Snapshot, int SnappingClient) = 0;
	virtual void Snap07(int Snapshot, int SnappingClient) = 0;
	virtual void Snap07ModAPI(int Snapshot, int SnappingClient);

protected:
	int GetID(int Snapshot, int ID);
};

class CModAPI_Entity_SnapshotModAPI : public CEntity
{
private:
	array<int> m_IDList06;
	array<int> m_IDList07;
	array<int> m_IDList07ModAPI;
	
public:
	CModAPI_Entity_SnapshotModAPI(CGameWorld *pGameWorld, int Objtype, vec2 Pos, int ProximityRadius=0, int NbId06=1, int NbId07=1, int NbId07ModAPI=1);
	~CModAPI_Entity_SnapshotModAPI();
		
	virtual void Snap06(int Snapshot, int SnappingClient) = 0;
	virtual void Snap07(int Snapshot, int SnappingClient) = 0;
	virtual void Snap07ModAPI(int Snapshot, int SnappingClient) = 0;
	
protected:
	int GetID(int Snapshot, int ID);
};

#endif
