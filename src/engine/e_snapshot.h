/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_SNAPSHOT_H
#define ENGINE_SNAPSHOT_H

#include <base/system.h>

/* CSnapshot */



class CSnapshotItem
{
public:
	int m_TypeAndID;
	
	int *Data() { return (int *)(this+1); }
	int Type() { return m_TypeAndID>>16; }
	int ID() { return m_TypeAndID&0xffff; }
	int Key() { return m_TypeAndID; }
};

class CSnapshotDelta
{
public:
	int m_NumDeletedItems;
	int m_NumUpdateItems;
	int m_NumTempItems; /* needed? */
	int m_pData[1];
};

// TODO: hide a lot of these members
class CSnapshot
{
public:
	enum
	{
		MAX_SIZE=64*1024
	};

	int m_DataSize;
	int m_NumItems;
	
	int *Offsets() const { return (int *)(this+1); }
	char *DataStart() const { return (char*)(Offsets()+m_NumItems); }
	CSnapshotItem *GetItem(int Index);
	int GetItemSize(int Index);
	int GetItemIndex(int Key);

	int Crc();
	void DebugDump();

	// TODO: move these
	int GetItemIndexHashed(int Key);
	int GenerateHash();
	
	//
	static CSnapshotDelta *EmptyDelta();
	static int CreateDelta(CSnapshot *pFrom, CSnapshot *pTo, void *pData);
	static int UnpackDelta(CSnapshot *pFrom, CSnapshot *pTo, void *pData, int DataSize);
};

/* CSnapshotStorage */


class CSnapshotStorage
{
public:
	class CHolder
	{
	public:
		CHolder *m_pPrev;
		CHolder *m_pNext;
		
		int64 m_Tagtime;
		int m_Tick;
		
		int m_SnapSize;
		CSnapshot *m_pSnap;
		CSnapshot *m_pAltSnap;
	};
	 

	CHolder *m_pFirst;
	CHolder *m_pLast;

	void Init();
	void PurgeAll();
	void PurgeUntil(int Tick);
	void Add(int Tick, int64 Tagtime, int DataSize, void *pData, int CreateAlt);
	int Get(int Tick, int64 *Tagtime, CSnapshot **pData, CSnapshot **ppAltData);
};

class CSnapshotBuilder
{
	enum
	{
		MAX_ITEMS = 1024*2
	};

	char m_aData[CSnapshot::MAX_SIZE];
	int m_DataSize;

	int m_aOffsets[MAX_ITEMS];
	int m_NumItems;

public:
	void Init();
	
	void *NewItem(int Type, int ID, int Size);
	
	CSnapshotItem *GetItem(int Index);
	int *GetItemData(int Key);
	
	int Finish(void *Snapdata);
};


#endif /* ENGINE_SNAPSHOT_H */
