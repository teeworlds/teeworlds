#include "linereader.h"

void CLineReader::Init(IOHANDLE io)
{
	m_BufferMaxSize = 4*1024;
	m_BufferSize = 0;
	m_BufferPos = 0;
	m_IO = io;
}

char *CLineReader::Get()
{
	unsigned LineStart = m_BufferPos;

	while(1)
	{
		if(m_BufferPos >= m_BufferSize)
		{
			// fetch more

			// move the remaining part to the front
			unsigned Read;
			unsigned Left = m_BufferSize - LineStart;

			if(LineStart > m_BufferSize)
				Left = 0;
			if(Left)
				mem_move(m_aBuffer, &m_aBuffer[LineStart], Left);
			m_BufferPos = Left;

			// fill the buffer
			Read = io_read(m_IO, &m_aBuffer[m_BufferPos], m_BufferMaxSize-m_BufferPos);
			m_BufferSize = Left + Read;
			LineStart = 0;

			if(!Read)
			{
				if(Left)
				{
					m_aBuffer[Left] = 0; // return the last line
					m_BufferPos = Left;
					m_BufferSize = Left;
					return m_aBuffer;
				}
				else
					return 0x0; // we are done!
			}
		}
		else
		{
			if(m_aBuffer[m_BufferPos] == '\n' || m_aBuffer[m_BufferPos] == '\r')
			{
				// line found
				if(m_aBuffer[m_BufferPos] == '\r' && m_BufferPos+1 < m_BufferSize && m_aBuffer[m_BufferPos+1] == '\n')
					m_aBuffer[m_BufferPos++] = 0;
				m_aBuffer[m_BufferPos] = 0;
				m_BufferPos++;
				return &m_aBuffer[LineStart];
			}
			else
				m_BufferPos++;
		}
	}
}
