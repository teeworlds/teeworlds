#pragma once

#include "class.h"

class CSmoker : public IClass {
private:
	int m_BodyColor;
	int m_MarkingColor;
	int m_FeetColor;
	char m_MarkingName[24];
public:
	CSmoker();
	~CSmoker() override;
	int GetBodyColor() const override;
	int GetMarkingColor() const override;
	int GetFeetColor() const override;
	const char* GetMarkingName() const override;

	void OnCharacterSpawn(CCharacter* pChr) override;
};