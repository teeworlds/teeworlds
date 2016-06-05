#ifndef MODAPI_CLIENT_ASSETS_SKELETONANIMATION_H
#define MODAPI_CLIENT_ASSETS_SKELETONANIMATION_H

#include <modapi/client/assets.h>

#define MODAPI_SKELETONANIMATION_TIMESTEP 30

class CModAPI_Asset_SkeletonAnimation : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_SKELETONANIMATION;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		struct CBoneKeyFrame
		{
			int m_Time;
			float m_TranslationX;
			float m_TranslationY;
			float m_ScaleX;
			float m_ScaleY;
			float m_Angle;
			int m_Alignment;
		};
		
		struct CBoneAnimation
		{
			int m_BonePath;
			int m_CycleType;
			int m_FirstKeyFrame;
			int m_NumKeyFrames;
		};
		
		struct CLayerKeyFrame
		{
			int m_Time;
			int m_Color;
			int m_State;
		};
		
		struct CLayerAnimation
		{
			int m_LayerPath;
			int m_CycleType;
			int m_FirstKeyFrame;
			int m_NumKeyFrames;
		};
		
		int m_SkeletonPath;
		
		int m_NumBoneKeyFrames;
		int m_NumBoneAnimations;
		int m_NumLayerKeyFrames;
		int m_NumLayerAnimations;
		
		int m_BoneKeyFramesData;
		int m_BoneAnimationsData;
		int m_LayerKeyFramesData;
		int m_LayerAnimationsData;
		
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);
	
public:
	class CSubPath : public CModAPI_GenericPath<2, 0, 0, 16>
	{
	public:
		enum
		{
			TYPE_BONEANIMATION=0,
			TYPE_BONEKEYFRAME,
			TYPE_LAYERANIMATION,
			TYPE_LAYERKEYFRAME,
			NUM_TYPES,
		};
		
	public:
		CSubPath() : CModAPI_GenericPath() { }
		CSubPath(int PathInt) : CModAPI_GenericPath(PathInt) { }
		CSubPath(int Type, int Id, int Id2) : CModAPI_GenericPath(Type, 0, 0x0, Id, Id2) { }
		
		static inline CSubPath Null() { return CSubPath(CModAPI_GenericPath::UNDEFINED); }
		static inline CSubPath BoneAnimation(int Id) { return CSubPath(TYPE_BONEANIMATION, Id, 0); }
		static inline CSubPath BoneKeyFrame(int Id, int Id2) { return CSubPath(TYPE_BONEKEYFRAME, Id, Id2); }
		static inline CSubPath LayerAnimation(int Id) { return CSubPath(TYPE_LAYERANIMATION, Id, 0); }
		static inline CSubPath LayerKeyFrame(int Id, int Id2) { return CSubPath(TYPE_LAYERKEYFRAME, Id, Id2); }
	};

public:
	enum
	{
		CYCLETYPE_CLAMP = 0,
		CYCLETYPE_LOOP,
		NUM_CYCLETYPES,
	};
	
	enum
	{
		LAYERSTATE_VISIBLE = 0,
		LAYERSTATE_HIDDEN,
		NUM_LAYERSTATES,
	};
	
	enum
	{
		BONEALIGN_PARENTBONE = 0,
		BONEALIGN_WORLD,
		BONEALIGN_AIM,
		BONEALIGN_MOTION,
		BONEALIGN_HOOK,
		NUM_BONEALIGNS,
	};

	class CBoneAnimation
	{
	public:
		class CFrame
		{
		public:
			vec2 m_Translation;
			vec2 m_Scale;
			float m_Angle;
			int m_Alignment;
			
			CFrame() :
				m_Translation(vec2(0.0f, 0.0f)),
				m_Scale(vec2(1.0f, 1.0f)),
				m_Angle(0),
				m_Alignment(BONEALIGN_PARENTBONE)
			{
				
			}
		};
		
		class CKeyFrame : public CFrame
		{
		public:
			int m_Time; // 1 = 1/50 sec
			
			CKeyFrame& Angle(float v)
			{
				m_Angle = v;
				return *this;
			}
			
			CKeyFrame& Translation(vec2 v)
			{
				m_Translation = v;
				return *this;
			}
			
			CKeyFrame& Alignment(int v)
			{
				m_Alignment = v;
				return *this;
			}
		};
		
		CModAPI_Asset_Skeleton::CBonePath m_BonePath;
		array<CKeyFrame> m_KeyFrames;
		int m_CycleType;
		
		CBoneAnimation() :
			m_CycleType(CYCLETYPE_CLAMP),
			m_BonePath(CModAPI_Asset_Skeleton::CBonePath::Null())
		{
			
		}
		
		inline int IntTimeToKeyFrame(int IntTime) const
		{
			if(m_KeyFrames.size() == 0)
				return 0;
			
			int i;
			for(i=0; i<m_KeyFrames.size(); i++)
			{
				if(m_KeyFrames[i].m_Time > IntTime)
					break;
			}
			
			return i;
		}
		
		inline int TimeToKeyFrame(float Time) const
		{
			return IntTimeToKeyFrame(Time*MODAPI_SKELETONANIMATION_TIMESTEP);
		}

		float GetDuration() const
		{
			if(m_KeyFrames.size())
				return m_KeyFrames[m_KeyFrames.size()-1].m_Time/static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP);
			else
				return 1.0f;
		}

		inline bool GetFrame(float Time, CFrame* pFrame) const
		{
			float CycleTime = Time;
			if(m_CycleType == CYCLETYPE_LOOP)
			{
				CycleTime = fmod(Time, GetDuration());
			}
			
			int i = TimeToKeyFrame(CycleTime);
			
			if(i == m_KeyFrames.size())
			{
				if(i == 0)
					return false;
				else
					*pFrame = m_KeyFrames[m_KeyFrames.size()-1];
			}
			else if(i == 0)
			{
				*pFrame = m_KeyFrames[0];
			}
			else
			{
				float alpha = (MODAPI_SKELETONANIMATION_TIMESTEP*CycleTime - m_KeyFrames[i-1].m_Time) / (m_KeyFrames[i].m_Time - m_KeyFrames[i-1].m_Time);
				pFrame->m_Translation = mix(m_KeyFrames[i-1].m_Translation, m_KeyFrames[i].m_Translation, alpha);
				pFrame->m_Scale = mix(m_KeyFrames[i-1].m_Scale, m_KeyFrames[i].m_Scale, alpha);
				pFrame->m_Angle = mix(m_KeyFrames[i-1].m_Angle, m_KeyFrames[i].m_Angle, alpha); //Need better interpolation
				pFrame->m_Alignment = m_KeyFrames[i-1].m_Alignment;
			}
			
			return true;
		}
	};

	class CLayerAnimation
	{
	public:
		class CFrame
		{
		public:
			vec4 m_Color;
			int m_State;
			
			CFrame() :
				m_Color(vec4(1.0f, 1.0f, 1.0f, 1.0f)),
				m_State(0)
			{
				
			}
		};
		
		class CKeyFrame : public CFrame
		{
		public:
			int m_Time; // 1 = 1/50 sec
			
			CKeyFrame& State(int v)
			{
				m_State = v;
				return *this;
			}
			
			CKeyFrame& Color(vec4 v)
			{
				m_Color = v;
				return *this;
			}
		};
		
		CModAPI_Asset_Skeleton::CBonePath m_LayerPath;
		array<CKeyFrame> m_KeyFrames;
		int m_CycleType;
		
		CLayerAnimation() :
			m_CycleType(CYCLETYPE_CLAMP),
			m_LayerPath(CModAPI_Asset_Skeleton::CBonePath::Null())
		{
			
		}
		
		inline int IntTimeToKeyFrame(int IntTime) const
		{
			if(m_KeyFrames.size() == 0)
				return 0;
			
			int i;
			for(i=0; i<m_KeyFrames.size(); i++)
			{
				if(m_KeyFrames[i].m_Time > IntTime)
					break;
			}
			
			return i;
		}
		
		inline int TimeToKeyFrame(float Time) const
		{
			return IntTimeToKeyFrame(Time*MODAPI_SKELETONANIMATION_TIMESTEP);
		}

		float GetDuration() const
		{
			if(m_KeyFrames.size())
				return m_KeyFrames[m_KeyFrames.size()-1].m_Time/static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP);
			else
				return 1.0f;
		}

		inline bool GetFrame(float Time, CFrame* pFrame) const
		{
			float CycleTime = Time;
			if(m_CycleType == CYCLETYPE_LOOP)
			{
				CycleTime = fmod(Time, GetDuration());
			}
			
			int i = TimeToKeyFrame(CycleTime);
			
			if(i == m_KeyFrames.size())
			{
				if(i == 0)
					return false;
				else
					*pFrame = m_KeyFrames[m_KeyFrames.size()-1];
			}
			else if(i == 0)
			{
				*pFrame = m_KeyFrames[0];
			}
			else
			{
				float alpha = (MODAPI_SKELETONANIMATION_TIMESTEP*CycleTime - m_KeyFrames[i-1].m_Time) / (m_KeyFrames[i].m_Time - m_KeyFrames[i-1].m_Time);
				pFrame->m_Color = mix(m_KeyFrames[i-1].m_Color, m_KeyFrames[i].m_Color, alpha); //Need better interpolation
				pFrame->m_State = m_KeyFrames[i].m_State;
			}
			
			return true;
		}
	};
	

public:
	CModAPI_AssetPath m_SkeletonPath;
	array<CBoneAnimation> m_BoneAnimations;
	array<CLayerAnimation> m_LayerAnimations;

public:
	CModAPI_Asset_SkeletonAnimation()
	{
		
	}
	
	CBoneAnimation::CKeyFrame& AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath BonePath, int Time)
	{
		//Find BoneAnimation
		int BoneAnimationId = -1;
		for(int i=0; i<m_BoneAnimations.size(); i++)
		{
			if(m_BoneAnimations[i].m_BonePath == BonePath)
			{
				BoneAnimationId = i;
				break;
			}
		}
		if(BoneAnimationId < 0)
		{
			BoneAnimationId = m_BoneAnimations.size();
			m_BoneAnimations.set_size(m_BoneAnimations.size()+1);
			m_BoneAnimations[BoneAnimationId].m_BonePath = BonePath;
		}
		
		CModAPI_Asset_SkeletonAnimation::CBoneAnimation::CKeyFrame KeyFrame;
		m_BoneAnimations[BoneAnimationId].GetFrame(Time/static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP), &KeyFrame);
		KeyFrame.m_Time = Time;
		
		int KeyFrameId = m_BoneAnimations[BoneAnimationId].IntTimeToKeyFrame(Time);
		if(KeyFrameId == m_BoneAnimations[BoneAnimationId].m_KeyFrames.size())
		{
			m_BoneAnimations[BoneAnimationId].m_KeyFrames.add(KeyFrame);
		}
		else
		{
			m_BoneAnimations[BoneAnimationId].m_KeyFrames.insertat(KeyFrame, KeyFrameId);
		}
		return m_BoneAnimations[BoneAnimationId].m_KeyFrames[KeyFrameId];
	}
	
	CLayerAnimation::CKeyFrame& AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath LayerPath, int Time)
	{
		//Find BoneAnimation
		int LayerAnimationId = -1;
		for(int i=0; i<m_LayerAnimations.size(); i++)
		{
			if(m_LayerAnimations[i].m_LayerPath == LayerPath)
			{
				LayerAnimationId = i;
				break;
			}
		}
		if(LayerAnimationId < 0)
		{
			LayerAnimationId = m_LayerAnimations.size();
			m_LayerAnimations.set_size(m_LayerAnimations.size()+1);
			m_LayerAnimations[LayerAnimationId].m_LayerPath = LayerPath;
		}
		
		CModAPI_Asset_SkeletonAnimation::CLayerAnimation::CKeyFrame KeyFrame;
		m_LayerAnimations[LayerAnimationId].GetFrame(Time/static_cast<float>(MODAPI_SKELETONANIMATION_TIMESTEP), &KeyFrame);
		KeyFrame.m_Time = Time;
		
		int KeyFrameId = m_LayerAnimations[LayerAnimationId].IntTimeToKeyFrame(Time);
		if(KeyFrameId == m_LayerAnimations[LayerAnimationId].m_KeyFrames.size())
		{
			m_LayerAnimations[LayerAnimationId].m_KeyFrames.add(KeyFrame);
		}
		else
		{
			m_LayerAnimations[LayerAnimationId].m_KeyFrames.insertat(KeyFrame, KeyFrameId);
		}
		
		return m_LayerAnimations[LayerAnimationId].m_KeyFrames[KeyFrameId];
	}
	
	void SetCycle(CModAPI_Asset_Skeleton::CBonePath BonePath, int CycleType)
	{
		//Find BoneAnimation
		for(int i=0; i<m_BoneAnimations.size(); i++)
		{
			if(m_BoneAnimations[i].m_BonePath == BonePath)
			{
				m_BoneAnimations[i].m_CycleType = CycleType;
				return;
			}
		}
	}
	
	CSubPath GetBoneKeyFramePath(CModAPI_Asset_Skeleton::CBonePath BonePath, int IntTime)
	{
		//Find BoneAnimation
		for(int i=0; i<m_BoneAnimations.size(); i++)
		{
			if(m_BoneAnimations[i].m_BonePath == BonePath)
			{
				for(int j=0; j<m_BoneAnimations[i].m_KeyFrames.size(); j++)
				{
					if(m_BoneAnimations[i].m_KeyFrames[j].m_Time == IntTime)
					{
						return CSubPath::BoneKeyFrame(i, j);
					}
				}
				return CSubPath::Null();
			}
		}
		return CSubPath::Null();
	}
	
	CSubPath GetBoneKeyFramePath(CModAPI_Asset_SkeletonAnimation::CSubPath SubPath, int IntTime)
	{
		if(SubPath.GetType() != CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_BONEANIMATION)
			return CSubPath::Null();
			
		int animId = SubPath.GetId();
		if(animId < 0 || animId >= m_BoneAnimations.size())
			return CSubPath::Null();
		
		for(int i=0; i<m_BoneAnimations[animId].m_KeyFrames.size(); i++)
		{
			if(m_BoneAnimations[animId].m_KeyFrames[i].m_Time == IntTime)
			{
				return CSubPath::BoneKeyFrame(animId, i);
			}
		}
		return CSubPath::Null();
	}
	
	CSubPath GetLayerKeyFramePath(CModAPI_Asset_Skeleton::CBonePath LayerPath, int IntTime)
	{
		//Find BoneAnimation
		for(int i=0; i<m_LayerAnimations.size(); i++)
		{
			if(m_LayerAnimations[i].m_LayerPath == LayerPath)
			{
				for(int j=0; j<m_LayerAnimations[i].m_KeyFrames.size(); j++)
				{
					if(m_LayerAnimations[i].m_KeyFrames[j].m_Time == IntTime)
					{
						return CSubPath::LayerKeyFrame(i, j);
					}
				}
				return CSubPath::Null();
			}
		}
		return CSubPath::Null();
	}
	
	CSubPath GetLayerKeyFramePath(CModAPI_Asset_SkeletonAnimation::CSubPath SubPath, int IntTime)
	{
		if(SubPath.GetType() != CModAPI_Asset_SkeletonAnimation::CSubPath::TYPE_LAYERANIMATION)
			return CSubPath::Null();
			
		int animId = SubPath.GetId();
		if(animId < 0 || animId >= m_LayerAnimations.size())
			return CSubPath::Null();
		
		for(int i=0; i<m_LayerAnimations[animId].m_KeyFrames.size(); i++)
		{
			if(m_LayerAnimations[animId].m_KeyFrames[i].m_Time == IntTime)
			{
				return CSubPath::LayerKeyFrame(animId, i);
			}
		}
		return CSubPath::Null();
	}

	//Getter/Setter for AssetsEditor
public:
	enum
	{
		SKELETONPATH = CModAPI_Asset::NUM_MEMBERS, //AssetPath
		BONEANIMATION_CYCLETYPE, //Int
		BONEKEYFRAME_TIME, //Int
		BONEKEYFRAME_TRANSLATION_X, //Float
		BONEKEYFRAME_TRANSLATION_Y, //Float
		BONEKEYFRAME_SCALE_X, //Float
		BONEKEYFRAME_SCALE_Y, //Float
		BONEKEYFRAME_ANGLE, //Float
		BONEKEYFRAME_ALIGNMENT, //Int
		LAYERKEYFRAME_TIME, //Int
		LAYERKEYFRAME_COLOR, //Color
		LAYERKEYFRAME_STATE, //Int
	};
	
	template<typename T>
	T GetValue(int ValueType, int Path, T DefaultValue)
	{
		return CModAPI_Asset::GetValue<T>(ValueType, Path, DefaultValue);
	}
	
	template<typename T>
	bool SetValue(int ValueType, int Path, T Value)
	{
		return CModAPI_Asset::SetValue<T>(ValueType, Path, Value);
	}
	
	inline int GetKeyFramePath(int i)
	{
		return i;
	}
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path)
	{
		m_SkeletonPath.OnIdDeleted(Path);
	}
	
	inline int AddSubItem(int SubItemType) { }
	
	inline bool DeleteSubItem(CSubPath SubItemPath)
	{
		switch(SubItemPath.GetType())
		{
			case CSubPath::TYPE_BONEKEYFRAME:
			{
				int animId = SubItemPath.GetId();
				if(animId >= 0 && animId < m_BoneAnimations.size())
				{
					int keyId = SubItemPath.GetId2();
					if(keyId >= 0 && keyId < m_BoneAnimations[animId].m_KeyFrames.size())
					{
						m_BoneAnimations[animId].m_KeyFrames.remove_index(keyId);
						if(m_BoneAnimations.size() == 0)
						{
							m_BoneAnimations.remove_index(animId);
						}
					}
				}
				return true;
			}
			case CSubPath::TYPE_LAYERKEYFRAME:
			{
				int animId = SubItemPath.GetId();
				if(animId >= 0 && animId < m_LayerAnimations.size())
				{
					int keyId = SubItemPath.GetId2();
					if(keyId >= 0 && keyId < m_LayerAnimations[animId].m_KeyFrames.size())
					{
						m_LayerAnimations[animId].m_KeyFrames.remove_index(keyId);
						if(m_LayerAnimations.size() == 0)
						{
							m_LayerAnimations.remove_index(animId);
						}
					}
				}
				return true;
			}
		}
		
		return false;
	}
	
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
