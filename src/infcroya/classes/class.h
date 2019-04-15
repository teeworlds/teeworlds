#pragma once

class IClass {
public:
	virtual ~IClass() {};
	virtual int GetBodyColor() const = 0;
	virtual int GetMarkingColor() const = 0;
	virtual int GetFeetColor() const = 0;
	virtual const char* GetMarkingName() const = 0;

	virtual void OnCharacterSpawn(class CCharacter* pChr) = 0;

	int HSLtoInt(int H, int S, int L) {
		int color = 0;
		color = (color & 0xFF00FFFF) | (H << 16);
		color = (color & 0xFFFF00FF) | (S << 8);
		color = (color & 0xFFFFFF00) | L;
		color = (color & 0x00FFFFFF) | (255 << 24);
		return color;
	}
};