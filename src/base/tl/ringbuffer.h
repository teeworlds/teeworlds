#pragma once

#include <base/tl/base.h>
#include "threading.h"

/*
	RingBuffer - Multiple Writers, Single Reader

	Behaviours:
		* Any thread can write to it (Push)
		* One thread can read from it (Pop)
		* Push blocks if the buffer is full
*/
template<class TTYPE, int TSIZE>
class ringbuffer_mwsr
{
	TTYPE data[TSIZE];
	volatile unsigned put; // multiple writers
	volatile unsigned write; // multiple writers
	volatile unsigned get; // single writer

	enum
	{
		MASK = TSIZE-1,
	};

public:
	explicit ringbuffer_mwsr()
	{
		put = write = get = 0;
	}

	TTYPE pop()
	{
		//assert(NumElements());
		TTYPE item = data[get&MASK];
		sync_barrier();
		get++;
		return item;
	}

	void push(TTYPE item)
	{
		unsigned idx = atomic_inc(&write);

		// wait for it to become available
		while(idx-get >= TSIZE) { thread_yield(); }

		// copy the data
		mem_copy(&data[idx&MASK], &item, sizeof(TTYPE));
		sync_barrier();

		// wait for our turn to increase the put count
		while(put != idx) { thread_yield(); }
		atomic_inc(&put);
	}

	unsigned size() const { return put - get; }
};

/*
	RingBuffer - Single Writer, Single Reader

	Behaviours:
		* Any thread can write to it (Push)
		* One thread can read from it (Pop)
		* Push returns false if buffer is full
*/
template<class TTYPE, int TSIZE>
class ringbuffer_swsr
{
	TTYPE data[TSIZE];
	unsigned put; // single writer
	unsigned get; // single writer

	enum
	{
		MASK = TSIZE-1,
	};

public:
	explicit ringbuffer_swsr()
	{
		put = get = 0;
	}

	TTYPE pop()
	{
		assert(size());
		TTYPE item = data[get&MASK];
		sync_barrier();
		get++;
		return item;
	}

	bool push(TTYPE item)
	{
		if(size() == TSIZE)
			return false;

		// copy the data
		mem_copy(&data[put&MASK], &item, sizeof(TTYPE));
		sync_barrier();
		put++;
		return true;
	}

	unsigned size() const { return put - get; }
};

