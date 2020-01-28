/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "jsonwriter.h"

static char EscapeJsonChar(char c)
{
	switch(c)
	{
	case '\"': return '\"';
	case '\\': return '\\';
	case '\b': return 'b';
	case '\n': return 'n';
	case '\r': return 'r';
	case '\t': return 't';
	// Don't escape '\f', who uses that. :)
	default: return 0;
	}
}

CJsonWriter::CJsonWriter(IOHANDLE IO)
{
	m_IO = IO;
	m_NumStates = 0; // no root created yet
	m_Indentation = 0;
}

CJsonWriter::~CJsonWriter()
{
	WriteInternal("\n");
	io_close(m_IO);
}

void CJsonWriter::BeginObject()
{
	dbg_assert(CanWriteDatatype(), "Cannot write object at this position");
	WriteIndent(false);
	WriteInternal("{");
	PushState(STATE_OBJECT);
}

void CJsonWriter::EndObject()
{
	dbg_assert(TopState()->m_Kind == STATE_OBJECT, "Cannot end object here");
	PopState();
	CompleteDataType();
	WriteIndent(true);
	WriteInternal("}");
	
}

void CJsonWriter::BeginArray()
{
	dbg_assert(CanWriteDatatype(), "Cannot write array at this position");
	WriteIndent(false);
	WriteInternal("[");
	PushState(STATE_ARRAY);
}

void CJsonWriter::EndArray()
{
	dbg_assert(TopState()->m_Kind == STATE_ARRAY, "Cannot end array here");
	PopState();
	CompleteDataType();
	WriteIndent(true);
	WriteInternal("]");
}

void CJsonWriter::WriteAttribute(const char *pName)
{
	dbg_assert(TopState()->m_Kind == STATE_OBJECT, "Attribute can only be written inside of objects");
	WriteIndent(false);
	WriteInternalEscaped(pName);
	WriteInternal(": ");
	PushState(STATE_ATTRIBUTE);
}

void CJsonWriter::WriteStrValue(const char *pValue)
{
	dbg_assert(CanWriteDatatype(), "Cannot write value at this position");
	WriteIndent(false);
	WriteInternalEscaped(pValue);
	CompleteDataType();
}

void CJsonWriter::WriteIntValue(int Value)
{
	dbg_assert(CanWriteDatatype(), "Cannot write value at this position");
	WriteIndent(false);
	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%d", Value);
	WriteInternal(aBuf);
	CompleteDataType();
}

void CJsonWriter::WriteBoolValue(bool Value)
{
	dbg_assert(CanWriteDatatype(), "Cannot write value at this position");
	WriteIndent(false);
	WriteInternal(Value ? "true" : "false");
	CompleteDataType();
}

void CJsonWriter::WriteNullValue()
{
	dbg_assert(CanWriteDatatype(), "Cannot write value at this position");
	WriteIndent(false);
	WriteInternal("null");
	CompleteDataType();
}

bool CJsonWriter::CanWriteDatatype()
{
	return m_NumStates == 0
		|| TopState()->m_Kind == STATE_ARRAY
		|| TopState()->m_Kind == STATE_ATTRIBUTE;
}

inline void CJsonWriter::WriteInternal(const char *pStr)
{
	io_write(m_IO, pStr, str_length(pStr));
}

void CJsonWriter::WriteInternalEscaped(const char *pStr)
{
	WriteInternal("\"");
	int UnwrittenFrom = 0;
	int Length = str_length(pStr);
	for(int i = 0; i < Length; i++)
	{
		char SimpleEscape = EscapeJsonChar(pStr[i]);
		// Assuming ASCII/UTF-8, exactly everything below 0x20 is a
		// control character.
		bool NeedsEscape = SimpleEscape || pStr[i] < 0x20;
		if(NeedsEscape)
		{
			if(i - UnwrittenFrom > 0)
			{
				io_write(m_IO, pStr + UnwrittenFrom, i - UnwrittenFrom);
			}

			if(SimpleEscape)
			{
				char aStr[2];
				aStr[0] = '\\';
				aStr[1] = SimpleEscape;
				io_write(m_IO, aStr, sizeof(aStr));
			}
			else
			{
				char aStr[7];
				str_format(aStr, sizeof(aStr), "\\u%04x", pStr[i]);
				WriteInternal(aStr);
			}
			UnwrittenFrom = i + 1;
		}
	}
	if(Length - UnwrittenFrom > 0)
	{
		io_write(m_IO, pStr + UnwrittenFrom, Length - UnwrittenFrom);
	}
	WriteInternal("\"");
}

void CJsonWriter::WriteIndent(bool EndElement)
{
	const bool NotRootOrAttribute = m_NumStates != 0
		&& TopState()->m_Kind != STATE_ATTRIBUTE;

	if(NotRootOrAttribute && !TopState()->m_Empty && !EndElement)
		WriteInternal(",");

	if(NotRootOrAttribute || EndElement)
		io_write_newline(m_IO);

	if(NotRootOrAttribute)
		for(int i = 0; i < m_Indentation; i++)
			WriteInternal("\t");
}

void CJsonWriter::PushState(unsigned char NewState)
{
	dbg_assert(m_NumStates < MAX_DEPTH, "max json depth exceeded");
	if(m_NumStates != 0)
	{
		m_aStates[m_NumStates - 1].m_Empty = false;
	}
	m_aStates[m_NumStates] = CState(NewState);
	m_NumStates++;
	if(NewState != STATE_ATTRIBUTE)
	{
		m_Indentation++;
	}
}

CJsonWriter::CState *CJsonWriter::TopState()
{
	dbg_assert(m_NumStates != 0, "json stack is empty");
	return &m_aStates[m_NumStates - 1];
}

unsigned char CJsonWriter::PopState()
{
	dbg_assert(m_NumStates != 0, "json stack is empty");
	m_NumStates--;
	if(m_aStates[m_NumStates].m_Kind != STATE_ATTRIBUTE)
	{
		m_Indentation--;
	}
	return m_aStates[m_NumStates].m_Kind;
}

void CJsonWriter::CompleteDataType()
{
	if(m_NumStates != 0 && m_aStates[m_NumStates - 1].m_Kind == STATE_ATTRIBUTE)
		PopState(); // automatically complete the attribute

	if(m_NumStates != 0)
		m_aStates[m_NumStates - 1].m_Empty = false;
}
