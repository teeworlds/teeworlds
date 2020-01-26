/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "jsonwriter.h"

CJsonWriter::CJsonWriter(IOHANDLE io)
{
	m_IO = io;
	m_pState = 0; // no root created yet
	m_Indentation = 0;
}

CJsonWriter::~CJsonWriter()
{
	io_close(m_IO);

	while(m_pState != 0)
	{
		CState *pState = m_pState;
		m_pState = m_pState->m_pParent;
		delete pState;
	}
}

void CJsonWriter::BeginObject()
{
	dbg_assert(CanWriteDatatype(), "Cannot write object at this position");
	WriteIndent(false);
	WriteInternal("{");
	PushState(OBJECT);
}

void CJsonWriter::EndObject()
{
	dbg_assert(m_pState != 0 && m_pState->m_State == OBJECT, "Cannot end object here");
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
	PushState(ARRAY);
}

void CJsonWriter::EndArray()
{
	dbg_assert(m_pState != 0 && m_pState->m_State == ARRAY, "Cannot end array here");
	PopState();
	CompleteDataType();
	WriteIndent(true);
	WriteInternal("]");
}

void CJsonWriter::BeginAttribute(const char *pName)
{
	dbg_assert(m_pState != 0 && m_pState->m_State == OBJECT, "Attribute can only be written inside of objects");
	WriteIndent(false);
	WriteInternal("\"");
	WriteInternal(pName);
	WriteInternal("\"");
	WriteInternal(" : ");
	PushState(ATTRIBUTE);
}

void CJsonWriter::WriteStrValue(const char *pValue)
{
	dbg_assert(CanWriteDatatype(), "Cannot write value at this position");
	WriteIndent(false);
	WriteInternal("\"");
	WriteInternal(pValue);
	WriteInternal("\"");
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
	return m_pState == 0
		|| m_pState->m_State == ARRAY
		|| m_pState->m_State == ATTRIBUTE;
}

inline void CJsonWriter::WriteInternal(const char *pStr)
{
	io_write(m_IO, pStr, str_length(pStr));
}

void CJsonWriter::WriteIndent(bool EndElement)
{
	const bool NotRootOrAttribute = m_pState != 0 && m_pState->m_State != ATTRIBUTE;

	if(NotRootOrAttribute && !m_pState->m_Empty && !EndElement)
		WriteInternal(",");

	if(NotRootOrAttribute || EndElement)
		io_write_newline(m_IO);

	if(NotRootOrAttribute)
		for(int i = 0; i < m_Indentation; i++)
			WriteInternal("\t");
}

void CJsonWriter::PushState(EState NewState)
{
	if(m_pState != 0)
		m_pState->m_Empty = false;
	m_pState = new CState(NewState, m_pState);
	if(NewState != ATTRIBUTE)
		m_Indentation++;
}

CJsonWriter::EState CJsonWriter::PopState()
{
	dbg_assert(m_pState != 0, "Stack is empty");
	EState Result = m_pState->m_State;
	CState *pToDelete = m_pState;
	m_pState = m_pState->m_pParent;
	delete pToDelete;
	if(Result != ATTRIBUTE)
		m_Indentation--;
	return Result;
}

void CJsonWriter::CompleteDataType()
{
	if(m_pState != 0 && m_pState->m_State == ATTRIBUTE)
		PopState(); // automatically complete the attribute

	if(m_pState != 0)
		m_pState->m_Empty = false;
}