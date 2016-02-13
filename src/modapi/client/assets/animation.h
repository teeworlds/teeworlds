#ifndef MODAPI_CLIENT_ASSETS_ANIMATION_H
#define MODAPI_CLIENT_ASSETS_ANIMATION_H

#include <modapi/client/assets.h>
	
class CModAPI_Asset_Animation : public CModAPI_Asset
{
public:
	static const int TypeId = MODAPI_ASSETTYPE_ANIMATION;
	static const int ListId = MODAPI_ASSETTYPE_ANIMATION_LIST;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_CycleType;
		int m_NumKeyFrame;
		int m_KeyFrameData;
	};
	
	void InitFromAssetsFile(class CModAPI_Client_Graphics* pModAPIGraphics, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_Client_Graphics* pModAPIGraphics);
	
public:
	struct CFrame
	{
		vec2 m_Pos;
		float m_Angle;
		float m_Opacity;
		int m_ListId;
	};
	
	struct CKeyFrame : public CFrame
	{
		float m_Time;
	};
	
public:	
	array<CKeyFrame> m_lKeyFrames;
	int m_CycleType; //MODAPI_ANIMCYCLETYPE_XXXXX

public:	
	CModAPI_Asset_Animation() :
		m_CycleType(MODAPI_ANIMCYCLETYPE_CLAMP)
	{
		
	}
	
	int GetKeyFrameId(float Time) const;
	void AddKeyFrame(CKeyFrame& Frame);
	void AddKeyFrame(float Time, vec2 Pos, float Angle, float Opacity, int ListId = 0);
	void MoveUpKeyFrame(int Id);
	void MoveDownKeyFrame(int Id);
	void DuplicateKeyFrame(int Id);
	void DeleteKeyFrame(int Id);
	void GetFrame(float Time, CFrame* pFrame) const;
	float GetDuration() const;
};

#endif
