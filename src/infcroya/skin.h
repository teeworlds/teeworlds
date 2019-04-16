#pragma once

class CSkin {
private:
	int m_BodyColor;
	int m_MarkingColor;
	int m_FeetColor;
	char m_MarkingName[24];
public:
	CSkin();
	~CSkin();

	int GetBodyColor() const;
	void SetBodyColor(int H, int S, int L);

	int GetMarkingColor() const;
	void SetMarkingColor(int H, int S, int L);

	int GetFeetColor() const;
	void SetFeetColor(int H, int S, int L);

	const char* GetMarkingName() const;
	void SetMarkingName(const char* name);

	static int HSLtoInt(int H, int S, int L);
};