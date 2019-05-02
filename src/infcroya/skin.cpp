#include "skin.h"
#include "base/system.h"

CSkin::CSkin()
{
	SetBodyColor(27, 111, 116);
	SetMarkingColor(0, 0, 0);
	SetDecorationColor(0, 0, 0);
	SetHandsColor(27, 117, 158);
	SetFeetColor(28, 135, 62);
	SetEyesColor(0, 0, 0);
	SetBodyName("standard");
	SetHandsName("standard");
	SetFeetName("standard");
	SetEyesName("standard");
}

CSkin::~CSkin()
{
}

int CSkin::GetBodyColor() const
{
	return m_BodyColor;
}

void CSkin::SetBodyColor(int H, int S, int L)
{
	m_BodyColor = HSLtoInt(H, S, L);
}

int CSkin::GetMarkingColor() const
{
	return m_MarkingColor;
}

void CSkin::SetMarkingColor(int H, int S, int L)
{
	m_MarkingColor = HSLtoInt(H, S, L);
}

int CSkin::GetDecorationColor() const
{
	return m_DecorationColor;;
}

void CSkin::SetDecorationColor(int H, int S, int L)
{
	m_DecorationColor = HSLtoInt(H, S, L);
}

int CSkin::GetHandsColor() const
{
	return m_HandsColor;
}

void CSkin::SetHandsColor(int H, int S, int L)
{
	m_HandsColor = HSLtoInt(H, S, L);
}

int CSkin::GetFeetColor() const
{
	return m_FeetColor;
}

void CSkin::SetFeetColor(int H, int S, int L)
{
	m_FeetColor = HSLtoInt(H, S, L);
}

int CSkin::GetEyesColor() const
{
	return m_EyesColor;
}

void CSkin::SetEyesColor(int H, int S, int L)
{
	m_EyesColor = HSLtoInt(H, S, L);
}

const char* CSkin::GetBodyName() const
{
	return m_BodyName;
}

void CSkin::SetBodyName(const char* name)
{
	char SkinName[STRING_LENGTH];
	str_format(SkinName, sizeof(SkinName), "%s", name);
	str_copy(m_BodyName, SkinName, STRING_LENGTH);
}

const char* CSkin::GetMarkingName() const
{
	return m_MarkingName;
}

void CSkin::SetMarkingName(const char* name)
{
	char SkinName[STRING_LENGTH];
	str_format(SkinName, sizeof(SkinName), "%s", name);
	str_copy(m_MarkingName, SkinName, STRING_LENGTH);
}

const char* CSkin::GetDecorationName() const
{
	return m_DecorationName;
}

void CSkin::SetDecorationName(const char* name)
{
	char SkinName[STRING_LENGTH];
	str_format(SkinName, sizeof(SkinName), "%s", name);
	str_copy(m_DecorationName, SkinName, STRING_LENGTH);
}

const char* CSkin::GetHandsName() const
{
	return m_HandsName;
}

void CSkin::SetHandsName(const char* name)
{
	char SkinName[STRING_LENGTH];
	str_format(SkinName, sizeof(SkinName), "%s", name);
	str_copy(m_HandsName, SkinName, STRING_LENGTH);
}

const char* CSkin::GetFeetName() const
{
	return m_FeetName;
}

void CSkin::SetFeetName(const char* name)
{
	char SkinName[STRING_LENGTH];
	str_format(SkinName, sizeof(SkinName), "%s", name);
	str_copy(m_FeetName, SkinName, STRING_LENGTH);
}

const char* CSkin::GetEyesName() const
{
	return m_EyesName;
}

void CSkin::SetEyesName(const char* name)
{
	char SkinName[STRING_LENGTH];
	str_format(SkinName, sizeof(SkinName), "%s", name);
	str_copy(m_EyesName, SkinName, STRING_LENGTH);
}

int CSkin::HSLtoInt(int H, int S, int L)
{
	int color = 0;
	color = (color & 0xFF00FFFF) | (H << 16);
	color = (color & 0xFFFF00FF) | (S << 8);
	color = (color & 0xFFFFFF00) | L;
	color = (color & 0x00FFFFFF) | (255 << 24);
	return color;
}