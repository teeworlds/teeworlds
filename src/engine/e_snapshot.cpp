/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdlib.h>
#include "e_snapshot.h"
#include "e_engine.h"
#include "e_compression.h"
#include "e_common_interface.h"


/* TODO: strange arbitrary number */
static short item_sizes[64] = {0};

void snap_set_staticsize(int itemtype, int size)
{
	item_sizes[itemtype] = size;
}

CSnapshotItem *CSnapshot::GetItem(int Index)
{
	return (CSnapshotItem *)(DataStart() + Offsets()[Index]);
}

int CSnapshot::GetItemSize(int Index)
{
    if(Index == m_NumItems-1)
        return (m_DataSize - Offsets()[Index]) - sizeof(CSnapshotItem);
    return (Offsets()[Index+1] - Offsets()[Index]) - sizeof(CSnapshotItem);
}

int CSnapshot::GetItemIndex(int Key)
{
    /* TODO: OPT: this should not be a linear search. very bad */
    for(int i = 0; i < m_NumItems; i++)
    {
        if(GetItem(i)->Key() == Key)
            return i;
    }
    return -1;
}

// TODO: clean up this
typedef struct 
{
	int num;
	int keys[64];
	int index[64];
} ITEMLIST;

static ITEMLIST sorted[256];

int CSnapshot::GenerateHash()
{
    for(int i = 0; i < 256; i++)
    	sorted[i].num = 0;
    	
    for(int i = 0; i < m_NumItems; i++)
    {
    	int Key = GetItem(i)->Key();
    	int HashID = ((Key>>8)&0xf0) | (Key&0xf);
    	if(sorted[HashID].num != 64)
    	{
			sorted[HashID].index[sorted[HashID].num] = i;
			sorted[HashID].keys[sorted[HashID].num] = Key;
			sorted[HashID].num++;
		}
	}
    return 0;
}

int CSnapshot::GetItemIndexHashed(int Key)
{
   	int HashID = ((Key>>8)&0xf0) | (Key&0xf);
   	for(int i = 0; i < sorted[HashID].num; i++)
   	{
   		if(sorted[HashID].keys[i] == Key)
   			return sorted[HashID].index[i];
	}
	
	return -1;
}



static const int MAX_ITEMS = 512;
static CSnapshotDelta empty = {0,0,0,{0}};

CSnapshotDelta *CSnapshot::EmptyDelta()
{
	return &empty;
}

int CSnapshot::Crc()
{
	int crc = 0;
	
	for(int i = 0; i < m_NumItems; i++)
	{
		CSnapshotItem *pItem = GetItem(i);
		int Size = GetItemSize(i);
		
		for(int b = 0; b < Size/4; b++)
			crc += pItem->Data()[b];
	}
	return crc;
}

void CSnapshot::DebugDump()
{
	dbg_msg("snapshot", "data_size=%d num_items=%d", m_DataSize, m_NumItems);
	for(int i = 0; i < m_NumItems; i++)
	{
		CSnapshotItem *pItem = GetItem(i);
		int Size = GetItemSize(i);
		dbg_msg("snapshot", "\ttype=%d id=%d", pItem->Type(), pItem->ID());
		for(int b = 0; b < Size/4; b++)
			dbg_msg("snapshot", "\t\t%3d %12d\t%08x", b, pItem->Data()[b], pItem->Data()[b]);
	}
}


// TODO: remove these
int snapshot_data_rate[0xffff] = {0};
int snapshot_data_updates[0xffff] = {0};
static int snapshot_current = 0;

static int diff_item(int *past, int *current, int *out, int size)
{
	int needed = 0;
	while(size)
	{
		*out = *current-*past;
		needed |= *out;
		out++;
		past++;
		current++;
		size--;
	}
	
	return needed;
}

static void undiff_item(int *past, int *diff, int *out, int size)
{
	while(size)
	{
		*out = *past+*diff;
		
		if(*diff == 0)
			snapshot_data_rate[snapshot_current] += 1;
		else
		{
			unsigned char buf[16];
			unsigned char *end = vint_pack(buf,  *diff);
			snapshot_data_rate[snapshot_current] += (int)(end - (unsigned char*)buf) * 8;
		}
		
		out++;
		past++;
		diff++;
		size--;
	}
}


/* TODO: OPT: this should be made much faster */
int CSnapshot::CreateDelta(CSnapshot *from, CSnapshot *to, void *dstdata)
{
	static PERFORMACE_INFO hash_scope = {"hash", 0};
	CSnapshotDelta *delta = (CSnapshotDelta *)dstdata;
	int *data = (int *)delta->m_pData;
	int i, itemsize, pastindex;
	CSnapshotItem *pFromItem;
	CSnapshotItem *pCurItem;
	CSnapshotItem *pPastItem;
	int count = 0;
	int size_count = 0;
	
	delta->m_NumDeletedItems = 0;
	delta->m_NumUpdateItems = 0;
	delta->m_NumTempItems = 0;
	
	perf_start(&hash_scope);
	to->GenerateHash();
	perf_end();

	/* pack deleted stuff */
	{
		static PERFORMACE_INFO scope = {"delete", 0};
		perf_start(&scope);
		
		for(i = 0; i < from->m_NumItems; i++)
		{
			pFromItem = from->GetItem(i);
			if(to->GetItemIndexHashed(pFromItem->Key()) == -1)
			{
				/* deleted */
				delta->m_NumDeletedItems++;
				*data = pFromItem->Key();
				data++;
			}
		}
		
		perf_end();
	}
	
	perf_start(&hash_scope);
	from->GenerateHash();
	perf_end();
	
	/* pack updated stuff */
	{
		static PERFORMACE_INFO scope = {"update", 0};
		int pastindecies[1024];
		perf_start(&scope);

		/* fetch previous indices */
		/* we do this as a separate pass because it helps the cache */
		{
			static PERFORMACE_INFO scope = {"find", 0};
			perf_start(&scope);
			for(i = 0; i < to->m_NumItems; i++)
			{
				pCurItem = to->GetItem(i);  /* O(1) .. O(n) */
				pastindecies[i] = from->GetItemIndexHashed(pCurItem->Key()); /* O(n) .. O(n^n)*/
			}
			perf_end();
		}		
			
		for(i = 0; i < to->m_NumItems; i++)
		{
			/* do delta */
			itemsize = to->GetItemSize(i); /* O(1) .. O(n) */
			pCurItem = to->GetItem(i);  /* O(1) .. O(n) */
			pastindex = pastindecies[i];
			
			if(pastindex != -1)
			{
				static PERFORMACE_INFO scope = {"diff", 0};
				int *item_data_dst = data+3;
				perf_start(&scope);
		
				pPastItem = from->GetItem(pastindex);
				
				if(item_sizes[pCurItem->Type()])
					item_data_dst = data+2;
				
				if(diff_item((int*)pPastItem->Data(), (int*)pCurItem->Data(), item_data_dst, itemsize/4))
				{
					
					*data++ = pCurItem->Type();
					*data++ = pCurItem->ID();
					if(!item_sizes[pCurItem->Type()])
						*data++ = itemsize/4;
					data += itemsize/4;
					delta->m_NumUpdateItems++;
				}
				perf_end();
			}
			else
			{
				static PERFORMACE_INFO scope = {"copy", 0};
				perf_start(&scope);
				
				*data++ = pCurItem->Type();
				*data++ = pCurItem->ID();
				if(!item_sizes[pCurItem->Type()])
					*data++ = itemsize/4;
				
				mem_copy(data, pCurItem->Data(), itemsize);
				size_count += itemsize;
				data += itemsize/4;
				delta->m_NumUpdateItems++;
				count++;
				
				perf_end();
			}
		}
		
		perf_end();
	}
	
	if(0)
	{
		dbg_msg("snapshot", "%d %d %d",
			delta->m_NumDeletedItems,
			delta->m_NumUpdateItems,
			delta->m_NumTempItems);
	}

	/*
	// TODO: pack temp stuff
	
	// finish
	//mem_copy(delta->offsets, deleted, delta->num_deleted_items*sizeof(int));
	//mem_copy(&(delta->offsets[delta->num_deleted_items]), update, delta->num_update_items*sizeof(int));
	//mem_copy(&(delta->offsets[delta->num_deleted_items+delta->num_update_items]), temp, delta->num_temp_items*sizeof(int));
	//mem_copy(delta->data_start(), data, data_size);
	//delta->data_size = data_size;
	* */
	
	if(!delta->m_NumDeletedItems && !delta->m_NumUpdateItems && !delta->m_NumTempItems)
		return 0;
	
	return (int)((char*)data-(char*)dstdata);
}

static int range_check(void *end, void *ptr, int size)
{
	if((const char *)ptr + size > (const char *)end)
		return -1;
	return 0;
}

int CSnapshot::UnpackDelta(CSnapshot *from, CSnapshot *to, void *srcdata, int data_size)
{
	CSnapshotBuilder builder;
	CSnapshotDelta *delta = (CSnapshotDelta *)srcdata;
	int *data = (int *)delta->m_pData;
	int *end = (int *)(((char *)srcdata + data_size));
	
	CSnapshotItem *fromitem;
	int i, d, keep, itemsize;
	int *deleted;
	int id, type, key;
	int fromindex;
	int *newdata;
			
	builder.Init();
	
	/* unpack deleted stuff */
	deleted = data;
	data += delta->m_NumDeletedItems;
	if(data > end)
		return -1;

	/* copy all non deleted stuff */
	for(i = 0; i < from->m_NumItems; i++)
	{
		/* dbg_assert(0, "fail!"); */
		fromitem = from->GetItem(i);
		itemsize = from->GetItemSize(i); 
		keep = 1;
		for(d = 0; d < delta->m_NumDeletedItems; d++)
		{
			if(deleted[d] == fromitem->Key())
			{
				keep = 0;
				break;
			}
		}
		
		if(keep)
		{
			/* keep it */
			mem_copy(
				builder.NewItem(fromitem->Type(), fromitem->ID(), itemsize),
				fromitem->Data(), itemsize);
		}
	}
		
	/* unpack updated stuff */
	for(i = 0; i < delta->m_NumUpdateItems; i++)
	{
		if(data+2 > end)
			return -1;
		
		type = *data++;
		id = *data++;
		if(item_sizes[type])
			itemsize = item_sizes[type];
		else
		{
			if(data+1 > end)
				return -2;
			itemsize = (*data++) * 4;
		}
		snapshot_current = type;
		
		if(range_check(end, data, itemsize) || itemsize < 0) return -3;
		
		key = (type<<16)|id;
		
		/* create the item if needed */
		newdata = builder.GetItemData(key);
		if(!newdata)
			newdata = (int *)builder.NewItem(key>>16, key&0xffff, itemsize);

		/*if(range_check(end, newdata, itemsize)) return -4;*/
			
		fromindex = from->GetItemIndex(key);
		if(fromindex != -1)
		{
			/* we got an update so we need to apply the diff */
			undiff_item((int *)from->GetItem(fromindex)->Data(), data, newdata, itemsize/4);
			snapshot_data_updates[snapshot_current]++;
		}
		else /* no previous, just copy the data */
		{
			mem_copy(newdata, data, itemsize);
			snapshot_data_rate[snapshot_current] += itemsize*8;
			snapshot_data_updates[snapshot_current]++;
		}
			
		data += itemsize/4;
	}
	
	/* finish up */
	return builder.Finish(to);
}

/* CSnapshotStorage */

void CSnapshotStorage::Init()
{
	m_pFirst = 0;
	m_pLast = 0;
}

void CSnapshotStorage::PurgeAll()
{
	CHolder *pHolder = m_pFirst;
	CHolder *pNext;

	while(pHolder)
	{
		pNext = pHolder->m_pNext;
		mem_free(pHolder);
		pHolder = pNext;
	}

	/* no more snapshots in storage */
	m_pFirst = 0;
	m_pLast = 0;
}

void CSnapshotStorage::PurgeUntil(int Tick)
{
	CHolder *pHolder = m_pFirst;
	CHolder *pNext;
	
	while(pHolder)
	{
		pNext = pHolder->m_pNext;
		if(pHolder->m_Tick >= Tick)
			return; /* no more to remove */
		mem_free(pHolder);
		
		/* did we come to the end of the list? */
        if (!pNext)
            break;

		m_pFirst = pNext;
		pNext->m_pPrev = 0x0;
		
		pHolder = pNext;
	}
	
	/* no more snapshots in storage */
	m_pFirst = 0;
	m_pLast = 0;
}

void CSnapshotStorage::Add(int Tick, int64 Tagtime, int DataSize, void *pData, int CreateAlt)
{
	/* allocate memory for holder + snapshot_data */
	int TotalSize = sizeof(CHolder)+DataSize;
	
	if(CreateAlt)
		TotalSize += DataSize;
	
	CHolder *pHolder = (CHolder *)mem_alloc(TotalSize, 1);
	
	/* set data */
	pHolder->m_Tick = Tick;
	pHolder->m_Tagtime = Tagtime;
	pHolder->m_SnapSize = DataSize;
	pHolder->m_pSnap = (CSnapshot*)(pHolder+1);
	mem_copy(pHolder->m_pSnap, pData, DataSize);

	if(CreateAlt) /* create alternative if wanted */
	{
		pHolder->m_pAltSnap = (CSnapshot*)(((char *)pHolder->m_pSnap) + DataSize);
		mem_copy(pHolder->m_pAltSnap, pData, DataSize);
	}
	else
		pHolder->m_pAltSnap = 0;
		
	
	/* link */
	pHolder->m_pNext = 0;
	pHolder->m_pPrev = m_pLast;
	if(m_pLast)
		m_pLast->m_pNext = pHolder;
	else
		m_pFirst = pHolder;
	m_pLast = pHolder;
}

int CSnapshotStorage::Get(int Tick, int64 *pTagtime, CSnapshot **ppData, CSnapshot **ppAltData)
{
	CHolder *pHolder = m_pFirst;
	
	while(pHolder)
	{
		if(pHolder->m_Tick == Tick)
		{
			if(pTagtime)
				*pTagtime = pHolder->m_Tagtime;
			if(ppData)
				*ppData = pHolder->m_pSnap;
			if(ppAltData)
				*ppData = pHolder->m_pAltSnap;
			return pHolder->m_SnapSize;
		}
		
		pHolder = pHolder->m_pNext;
	}
	
	return -1;
}

/* CSnapshotBuilder */

void CSnapshotBuilder::Init()
{
	m_DataSize = 0;
	m_NumItems = 0;
}

CSnapshotItem *CSnapshotBuilder::GetItem(int Index) 
{
	return (CSnapshotItem *)&(m_aData[m_aOffsets[Index]]);
}

int *CSnapshotBuilder::GetItemData(int key)
{
	int i;
	for(i = 0; i < m_NumItems; i++)
	{
		if(GetItem(i)->Key() == key)
			return (int *)GetItem(i)->Data();
	}
	return 0;
}

int CSnapshotBuilder::Finish(void *snapdata)
{
	/* flattern and make the snapshot */
	CSnapshot *pSnap = (CSnapshot *)snapdata;
	int OffsetSize = sizeof(int)*m_NumItems;
	pSnap->m_DataSize = m_DataSize;
	pSnap->m_NumItems = m_NumItems;
	mem_copy(pSnap->Offsets(), m_aOffsets, OffsetSize);
	mem_copy(pSnap->DataStart(), m_aData, m_DataSize);
	return sizeof(CSnapshot) + OffsetSize + m_DataSize;
}

void *CSnapshotBuilder::NewItem(int Type, int ID, int Size)
{
	CSnapshotItem *pObj = (CSnapshotItem *)(m_aData + m_DataSize);

	mem_zero(pObj, sizeof(CSnapshotItem) + Size);
	pObj->m_TypeAndID = (Type<<16)|ID;
	m_aOffsets[m_NumItems] = m_DataSize;
	m_DataSize += sizeof(CSnapshotItem) + Size;
	m_NumItems++;
	
	dbg_assert(m_DataSize < CSnapshot::MAX_SIZE, "too much data");
	dbg_assert(m_NumItems < MAX_ITEMS, "too many items");

	return pObj->Data();
}
