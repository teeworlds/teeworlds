#include "skin.h"
#include "base/system.h"

CSkin::CSkin()
{
	SetBodyColor(27, 111, 116);
	SetMarkingColor(0, 0, 0);
	SetFeetColor(28, 135, 62);
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

int CSkin::GetFeetColor() const
{
	return m_FeetColor;
}

void CSkin::SetFeetColor(int H, int S, int L)
{
	m_FeetColor = HSLtoInt(H, S, L);
}

const char* CSkin::GetMarkingName() const
{
	return m_MarkingName;
}

void CSkin::SetMarkingName(const char* name)
{
	char SkinName[24];
	str_format(SkinName, sizeof(SkinName), name);
	str_copy(m_MarkingName, SkinName, 24);
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