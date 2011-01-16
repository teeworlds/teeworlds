/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_LINEREADER_H
#define ENGINE_SHARED_LINEREADER_H
#include <base/system.h>

// buffered stream for reading lines, should perhaps be something smaller
class CLineReader
{
	char m_aBuffer[4*1024];
	unsigned m_BufferPos;
	unsigned m_BufferSize;
	unsigned m_BufferMaxSize;
	IOHANDLE m_IO;
public:
	void Init(IOHANDLE IoHandle);
	char *Get();
};
#endif
