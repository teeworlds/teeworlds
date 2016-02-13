#ifndef MODAPI_CLIENT_ASSETSCATALOG_H
#define MODAPI_CLIENT_ASSETSCATALOG_H

#include <base/tl/array.h>
#include <modapi/client/assets.h>
#include <modapi/client/assets/list.h>

template<class T>
class CModAPI_AssetCatalog
{
public:
	array<T> m_InternalAssets;
	array<T> m_ExternalAssets;
	
	array<CModAPI_Asset_List> m_InternalLists;
	array<CModAPI_Asset_List> m_ExternalLists;

public:
	CModAPI_AssetPath GetFinalAssetPath(const CModAPI_AssetPath& path, int ListId)
	{
		int Id = path.GetId();
		if(Id < 0 || ListId < 0) return CModAPI_AssetPath::Null();
			
		if(path.IsList())
		{
			if(path.IsExternal())
			{
				if(Id >= m_ExternalLists.size())
					return CModAPI_AssetPath::Null();
				
				CModAPI_Asset_List& List = m_ExternalLists[Id];
					
				int ListIdMod = ListId % List.m_Elements.size();
				
				if(!List.m_Elements[ListIdMod].IsList())
					return List.m_Elements[ListIdMod];
				else
					return CModAPI_AssetPath::Null();
			}
			else
			{
				if(Id >= m_InternalLists.size())
					return CModAPI_AssetPath::Null();
				
				CModAPI_Asset_List& List = m_InternalLists[Id];
					
				int ListIdMod = ListId % List.m_Elements.size();
				
				if(!List.m_Elements[ListIdMod].IsList())
					return List.m_Elements[ListIdMod];
				else
					return CModAPI_AssetPath::Null();
			}
		}
		else return path;
	}
	
	T* GetAsset(const CModAPI_AssetPath& path, int ListId = 0)
	{
		int Id = path.GetId();
		if(Id < 0 || ListId < 0) return 0;
			
		if(path.IsList())
		{
			if(path.IsExternal())
			{
				if(Id >= m_ExternalLists.size())
					return 0;
				
				CModAPI_Asset_List& List = m_ExternalLists[Id];
					
				int ListIdMod = ListId % List.m_Elements.size();
				
				if(!List.m_Elements[ListIdMod].IsList())
					return GetAsset(List.m_Elements[ListIdMod]);
				else
					return 0;
			}
			else
			{
				if(Id >= m_InternalLists.size())
					return 0;
				
				CModAPI_Asset_List& List = m_InternalLists[Id];
					
				int ListIdMod = ListId % List.m_Elements.size();
				
				if(!List.m_Elements[ListIdMod].IsList())
					return GetAsset(List.m_Elements[ListIdMod]);
				else
					return 0;
			}
		}
		else
		{
			if(path.IsExternal())
			{
				if(Id < m_ExternalAssets.size())
					return &m_ExternalAssets[Id];
				else
					return 0;
			}
			else
			{
				if(Id < m_InternalAssets.size())
					return &m_InternalAssets[Id];
				else
					return 0;
			}
		}
	}
	
	CModAPI_Asset_List* GetList(const CModAPI_AssetPath& path)
	{
		int Id = path.GetId();
		if(Id < 0 || !path.IsList()) return 0;
			
		if(path.IsExternal())
		{
			if(Id < m_ExternalLists.size())
				return &m_ExternalLists[Id];
			else
				return 0;
		}
		else
		{
			if(Id < m_InternalLists.size())
				return &m_InternalLists[Id];
			else
				return 0;
		}
	}
	
	T* NewInternalAsset(CModAPI_AssetPath* path)
	{
		int Id = m_InternalAssets.add(T());
		*path = CModAPI_AssetPath::Internal(Id);
		return &m_InternalAssets[Id];
	}
	
	T* NewExternalAsset(CModAPI_AssetPath* path)
	{
		int Id = m_ExternalAssets.add(T());
		*path = CModAPI_AssetPath::External(Id);
		return &m_ExternalAssets[Id];
	}
	
	T* NewAsset(const CModAPI_AssetPath& path)
	{
		int Id = path.GetId();
		if(Id < 0 || path.IsList()) return 0;
		
		if(path.IsExternal())
		{
			int Size = max(m_ExternalAssets.size(), Id+1);
			m_ExternalAssets.set_size(Size);
			
			return &m_ExternalAssets[Id];
		}
		else
		{
			int Size = max(m_InternalAssets.size(), Id+1);
			m_InternalAssets.set_size(Size);
			
			return &m_InternalAssets[Id];
		}
	}
	
	CModAPI_Asset_List* NewList(const CModAPI_AssetPath& path)
	{
		int Id = path.GetId();
		if(Id < 0 || ! path.IsList()) return 0;
		
		if(path.IsExternal())
		{
			int Size = max(m_ExternalLists.size(), Id+1);
			m_ExternalLists.set_size(Size);
			
			return &m_ExternalLists[Id];
		}
		else
		{
			int Size = max(m_InternalLists.size(), Id+1);
			m_InternalLists.set_size(Size);
			
			return &m_InternalLists[Id];
		}
	}
	
	void LoadFromAssetsFile(class CModAPI_Client_Graphics* pModAPIGraphics, IModAPI_AssetsFile* pAssetsFile)
	{
		{
			int Start, Num;
			pAssetsFile->GetType(T::TypeId, &Start, &Num);
			
			m_ExternalAssets.set_size(Num);
			
			for(int i = 0; i < Num; i++)
			{
				class T::CStorageType* pItem = (class T::CStorageType*) pAssetsFile->GetItem(Start+i, 0, 0);
				T* pAsset = &m_ExternalAssets[i];
				pAsset->InitFromAssetsFile(pModAPIGraphics, pAssetsFile, pItem);
			}
		}
		{
			int Start, Num;
			pAssetsFile->GetType(T::ListId, &Start, &Num);
			
			m_ExternalLists.set_size(Num);
			
			for(int i = 0; i < Num; i++)
			{
				class CModAPI_Asset_List::CStorageType* pItem = (class CModAPI_Asset_List::CStorageType*) pAssetsFile->GetItem(Start+i, 0, 0);
				CModAPI_Asset_List* pList = &m_ExternalLists[i];
				pList->InitFromAssetsFile(pModAPIGraphics, pAssetsFile, pItem);
			}
		}
	}
	
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter)
	{
		for(int i=0; i<m_ExternalAssets.size(); i++)
		{
			m_ExternalAssets[i].SaveInAssetsFile(pFileWriter, i);
		}
		for(int i=0; i<m_ExternalLists.size(); i++)
		{
			m_ExternalLists[i].SaveInAssetsFile(pFileWriter, i, T::ListId);
		}
	}
	
	void Unload(class CModAPI_Client_Graphics* pModAPIGraphics)
	{
		for(int i=0; i<m_ExternalAssets.size(); i++)
		{
			m_ExternalAssets[i].Unload(pModAPIGraphics);
		}
		m_ExternalAssets.clear();
		
		for(int i=0; i<m_ExternalLists.size(); i++)
		{
			m_ExternalLists[i].Unload(pModAPIGraphics);
		}
		m_ExternalLists.clear();
	}
};

#endif
