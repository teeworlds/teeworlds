/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ALLOC_H
#define GAME_SERVER_ALLOC_H

#include <new>

#define MACRO_ALLOC_HEAP() \
	public: \
	void *operator new(size_t Size) \
	{ \
		void *p = mem_alloc(Size, 1); \
		/*dbg_msg("", "++ %p %d", p, size);*/ \
		mem_zero(p, Size); \
		return p; \
	} \
	void operator delete(void *pPtr) \
	{ \
		/*dbg_msg("", "-- %p", p);*/ \
		mem_free(pPtr); \
	} \
	private:

#define MACRO_ALLOC_POOL_ID() \
	public: \
	void *operator new(size_t Size, int id); \
	void operator delete(void *p); \
	private:

#define MACRO_ALLOC_POOL_ID_IMPL(POOLTYPE, PoolSize) \
	static char ms_PoolData##POOLTYPE[PoolSize][sizeof(POOLTYPE)] = {{0}}; \
	static int ms_PoolUsed##POOLTYPE[PoolSize] = {0}; \
	void *POOLTYPE::operator new(size_t Size, int id) \
	{ \
		dbg_assert(sizeof(POOLTYPE) == Size, "size error"); \
		dbg_assert(!ms_PoolUsed##POOLTYPE[id], "already used"); \
		/*dbg_msg("pool", "++ %s %d", #POOLTYPE, id);*/ \
		ms_PoolUsed##POOLTYPE[id] = 1; \
		mem_zero(ms_PoolData##POOLTYPE[id], Size); \
		return ms_PoolData##POOLTYPE[id]; \
	} \
	void POOLTYPE::operator delete(void *p) \
	{ \
		int id = (POOLTYPE*)p - (POOLTYPE*)ms_PoolData##POOLTYPE; \
		dbg_assert(ms_PoolUsed##POOLTYPE[id], "not used"); \
		/*dbg_msg("pool", "-- %s %d", #POOLTYPE, id);*/ \
		ms_PoolUsed##POOLTYPE[id] = 0; \
		mem_zero(ms_PoolData##POOLTYPE[id], sizeof(POOLTYPE)); \
	}

#endif
