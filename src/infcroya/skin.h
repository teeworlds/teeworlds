#pragma once

class CSkin {
private:
	// many fields and getters/setters are more readable than map, I think
	static const int STRING_LENGTH = 24;
	int m_BodyColor;
	int m_MarkingColor;
	int m_DecorationColor;
	int m_HandsColor;
	int m_FeetColor;
	int m_EyesColor;
	char m_BodyName[STRING_LENGTH];
	char m_MarkingName[STRING_LENGTH];
	char m_DecorationName[STRING_LENGTH];
	char m_HandsName[STRING_LENGTH];
	char m_FeetName[STRING_LENGTH];
	char m_EyesName[STRING_LENGTH];
public:
	CSkin();
	~CSkin();

	int GetBodyColor() const;
	void SetBodyColor(int H, int S, int L);

	int GetMarkingColor() const;
	void SetMarkingColor(int H, int S, int L);

	int GetDecorationColor() const;
	void SetDecorationColor(int H, int S, int L);

	int GetHandsColor() const;
	void SetHandsColor(int H, int S, int L);

	int GetFeetColor() const;
	void SetFeetColor(int H, int S, int L);

	int GetEyesColor() const;
	void SetEyesColor(int H, int S, int L);

	const char* GetBodyName() const;
	void SetBodyName(const char* name);

	const char* GetMarkingName() const;
	void SetMarkingName(const char* name);

	const char* GetDecorationName() const;
	void SetDecorationName(const char* name);

	const char* GetHandsName() const;
	void SetHandsName(const char* name);

	const char* GetFeetName() const;
	void SetFeetName(const char* name);

	const char* GetEyesName() const;
	void SetEyesName(const char* name);

	static int HSLtoInt(int H, int S, int L);
};