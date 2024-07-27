/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_SNAPSHOT_H
#define ENGINE_SHARED_SNAPSHOT_H

#include <base/system.h>

// CSnapshot

class CSnapshotItem
{
	friend class CSnapshotBuilder;
	int m_TypeAndID;

	int *Data() { return (int *)(this+1); }

public:
	const int *Data() const { return (int *)(this+1); }
	int Type() const { return m_TypeAndID>>16; }
	int ID() const { return m_TypeAndID&0xffff; }
	int Key() const { return m_TypeAndID; }
	void SetKey(int Type, int ID) { m_TypeAndID = (Type<<16)|(ID&0xffff); }
	void Invalidate() { m_TypeAndID = -1; }
};


class CSnapshot
{
	friend class CSnapshotBuilder;
	int m_DataSize;
	int m_NumItems;

	int *SortedKeys() const { return (int *)(this+1); }
	int *Offsets() const { return (int *)(SortedKeys()+m_NumItems); }
	char *DataStart() const { return (char*)(Offsets()+m_NumItems); }

public:
	enum
	{
		MAX_TYPE = 0x7fff,
		MAX_ID = 0xffff,
		MAX_PARTS	= 64,
		MAX_SIZE	= MAX_PARTS*1024
	};

	void Clear() { m_DataSize = 0; m_NumItems = 0; }
	int NumItems() const { return m_NumItems; }
	const CSnapshotItem *GetItem(int Index) const;
	int GetItemSize(int Index) const;
	int GetItemIndex(int Key) const;
	void InvalidateItem(int Index);

	int Serialize(char *pDstData) const;

	int Crc() const;
	void DebugDump() const;
};


// CSnapshotDelta

class CSnapshotDelta
{
public:
	class CData
	{
	public:
		int m_NumDeletedItems;
		int m_NumUpdateItems;
		int m_NumTempItems; // needed?
		int m_aData[1];
	};

private:
	enum
	{
		MAX_NETOBJSIZES=64
	};
	short m_aItemSizes[MAX_NETOBJSIZES];
	int m_aSnapshotDataRate[CSnapshot::MAX_TYPE + 1];
	int m_aSnapshotDataUpdates[CSnapshot::MAX_TYPE + 1];
	CData m_Empty;

public:
	CSnapshotDelta();
	int GetDataRate(int Index) const { return m_aSnapshotDataRate[Index]; }
	int GetDataUpdates(int Index) const { return m_aSnapshotDataUpdates[Index]; }
	void SetStaticsize(int ItemType, int Size);
	const CData *EmptyDelta() const;
	int CreateDelta(const class CSnapshot *pFrom, class CSnapshot *pTo, void *pDstData);
	int UnpackDelta(const class CSnapshot *pFrom, class CSnapshot *pTo, const void *pSrcData, int DataSize);
};


// CSnapshotStorage

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

	~CSnapshotStorage();
	void Init();
	void PurgeAll();
	void PurgeUntil(int Tick);
	void Add(int Tick, int64 Tagtime, int DataSize, const void *pData, bool CreateAlt);
	int Get(int Tick, int64 *pTagtime, CSnapshot **ppData, CSnapshot **ppAltData) const;
};

class CSnapshotBuilder
{
	enum
	{
		MAX_ITEMS = 1024
	};

	char m_aData[CSnapshot::MAX_SIZE];
	int m_DataSize;

	int m_aOffsets[MAX_ITEMS];
	int m_NumItems;

public:
	void Init();
	void Init(const CSnapshot *pSnapshot);
	bool UnserializeSnap(const char *pSrcData, int SrcSize);

	void *NewItem(int Type, int ID, int Size);

	CSnapshotItem *GetItem(int Index) const;
	int *GetItemData(int Key) const;

	int Finish(void *pSnapdata);
};


#endif // ENGINE_SNAPSHOT_H
