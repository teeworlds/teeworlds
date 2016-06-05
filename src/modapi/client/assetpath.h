#ifndef MODAPI_CLIENT_ASSETPATH_H
#define MODAPI_CLIENT_ASSETPATH_H

template<int NBTYPES, int NBSRCS, int NBFLAGS, int ID2SIZE=0>
class CModAPI_GenericPath
{
public:
	int m_Path;
	
	enum
	{
		TYPESIZE = Log2<(NearestPowerOfTwo<NBTYPES>::result)>::result,
		SRCSIZE = Log2<(NearestPowerOfTwo<NBSRCS>::result)>::result,
		FLAGSIZE = Log2<(NearestPowerOfTwo<NBFLAGS>::result)>::result,
		IDSIZE = 32 - FLAGSIZE - SRCSIZE - TYPESIZE - ID2SIZE,
		
		MASK_TYPE = ((0x1 << TYPESIZE)-1),
		MASK_SRC = ((0x1 << SRCSIZE)-1)<<TYPESIZE,
		MASK_FLAGS = ((0x1 << FLAGSIZE)-1)<<(TYPESIZE+SRCSIZE),
		MASK_ID = ((0x1 << IDSIZE)-1)<<(TYPESIZE+SRCSIZE+FLAGSIZE),
		MASK_ID2 = (ID2SIZE > 0 ? ((0x1 << ID2SIZE)-1)<<(TYPESIZE+SRCSIZE+FLAGSIZE+IDSIZE) : 0x0),
		
		TYPE_SHIFT = 0,
		SRC_SHIFT = TYPESIZE,
		FLAGS_SHIFT = TYPESIZE+SRCSIZE,
		ID_SHIFT = TYPESIZE+SRCSIZE+FLAGSIZE,
		ID2_SHIFT = TYPESIZE+SRCSIZE+FLAGSIZE+IDSIZE,
		
		UNDEFINED = -1,
	};
	
public:
	CModAPI_GenericPath() : m_Path(UNDEFINED) { }
	CModAPI_GenericPath(int PathInt) : m_Path(PathInt) { }
	CModAPI_GenericPath(int Type, int Source, int Flags, int Id) : m_Path((Type << TYPE_SHIFT) + (Source << SRC_SHIFT) + (Flags << FLAGS_SHIFT) + (Id << ID_SHIFT)) { }
	CModAPI_GenericPath(int Type, int Source, int Flags, int Id, int Id2) : m_Path((Type << TYPE_SHIFT) + (Source << SRC_SHIFT) + (Flags << FLAGS_SHIFT) + (Id << ID_SHIFT) + (Id2 << ID2_SHIFT)) { }
	
	inline int ConvertToInteger() const { return m_Path; }
	inline int GetId() const { return (m_Path & MASK_ID) >> ID_SHIFT; }
	inline int GetId2() const { return (m_Path & MASK_ID2) >> ID2_SHIFT; }
	inline int GetType() const { return (m_Path & MASK_TYPE) >> TYPE_SHIFT; }
	inline int GetSource() const { return (m_Path & MASK_SRC) >> SRC_SHIFT; }
	inline int GetFlags() const { return (m_Path & MASK_FLAGS) >> FLAGS_SHIFT; }
	
	inline void SetId(int Id) { m_Path = m_Path & ~MASK_ID + (Id << ID_SHIFT); }
	inline void SetId2(int Id) { m_Path = m_Path & ~MASK_ID2 + (Id << ID2_SHIFT); }
	
	inline bool IsNull() const { return m_Path == UNDEFINED; }
	inline bool operator==(const CModAPI_GenericPath& path) const { return path.m_Path == m_Path; }
	
	inline void OnIdDeleted(const CModAPI_GenericPath& Path)
	{
		if(!IsNull() && (m_Path & ~MASK_ID) == (Path.m_Path & ~MASK_ID))
		{
			int DeletedId = Path.GetId();
			int CurrentId = GetId();
			
			if(CurrentId == DeletedId)
				m_Path = UNDEFINED;
			else if(CurrentId > DeletedId)
				SetId(CurrentId-1);
		}
	}
};

class CModAPI_AssetPath : public CModAPI_GenericPath<12, 4, 0>
{
public:
	enum
	{
		SRC_INTERNAL=0, //Asset provided by the client
		SRC_EXTERNAL,   //Asset provided by the server
		SRC_MAP,        //Asset provided by the map
		SRC_SKIN,       //Asset provided by the skin
		NUM_SOURCES
	};

	enum
	{
		TYPE_IMAGE = 0,
		TYPE_SPRITE,
		TYPE_ANIMATION,
		TYPE_TEEANIMATION,
		TYPE_ATTACH,
		TYPE_LINESTYLE,
		TYPE_SKELETON,
		TYPE_SKELETONANIMATION,
		TYPE_SKELETONSKIN,
		TYPE_CHARACTER,
		TYPE_CHARACTERPART,
		TYPE_LIST,
		NUM_ASSETTYPES,
	};

public:
	CModAPI_AssetPath() : CModAPI_GenericPath() { }
	CModAPI_AssetPath(int PathInt) : CModAPI_GenericPath(PathInt) { }
	CModAPI_AssetPath(int Type, int Source, int Id) : CModAPI_GenericPath(Type, Source, 0x0, Id) { }
	
	//Static constructors	
	static inline CModAPI_AssetPath Null() { return CModAPI_AssetPath(CModAPI_GenericPath::UNDEFINED); }
	
	static inline CModAPI_AssetPath Asset(int Type, int Source, int Id) { return CModAPI_AssetPath(Type, Source, Id); }
	
	static inline CModAPI_AssetPath Internal(int Type, int Id) { return Asset(Type, SRC_INTERNAL, Id); }
	static inline CModAPI_AssetPath External(int Type, int Id) { return Asset(Type, SRC_EXTERNAL, Id); }
	static inline CModAPI_AssetPath Skin(int Type, int Id) { return Asset(Type, SRC_SKIN, Id); }
	static inline CModAPI_AssetPath Map(int Type, int Id) { return Asset(Type, SRC_MAP, Id); }
	
	static inline int TypeToStoredType(int Type) { return Type+1; }
	static inline int StoredTypeToType(int StoredType) { return StoredType-1; }
};

#endif
