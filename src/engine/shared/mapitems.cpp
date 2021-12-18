/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/mapitems.h>

bool CMapItemChecker::IsValid(const CMapItemInfo *pItem, unsigned Size)
{
	if(!pItem || Size < sizeof(CMapItemInfo_v1) || pItem->m_Version < CMapItemInfo_v1::CURRENT_VERSION)
		return false;
	else if(pItem->m_Version == CMapItemInfo_v1::CURRENT_VERSION && Size == sizeof(CMapItemInfo_v1))
		return true;
	return Size >= sizeof(CMapItemInfo_v1);
}

bool CMapItemChecker::IsValid(const CMapItemImage *pItem, unsigned Size)
{
	if(!pItem || Size < sizeof(CMapItemImage_v1) || pItem->m_Version < CMapItemImage_v1::CURRENT_VERSION
		|| pItem->m_Width < CMapItemImage::MIN_DIMENSION || pItem->m_Height < CMapItemImage::MIN_DIMENSION
		|| pItem->m_Width > CMapItemImage::MAX_DIMENSION || pItem->m_Height > CMapItemImage::MAX_DIMENSION)
		return false;
	else if(pItem->m_Version == CMapItemImage_v1::CURRENT_VERSION && Size == sizeof(CMapItemImage_v1))
		return true;
	else if(pItem->m_Version == CMapItemImage_v2::CURRENT_VERSION && Size == sizeof(CMapItemImage_v2))
		return true;
	return Size >= sizeof(CMapItemImage_v2);
}

bool CMapItemChecker::IsValid(const CMapItemGroup *pItem, unsigned Size)
{
	if(!pItem || Size < sizeof(CMapItemGroup_v1) || pItem->m_Version < CMapItemGroup_v1::CURRENT_VERSION
		|| pItem->m_StartLayer < 0 || pItem->m_NumLayers < 0)
		return false;
	else if(pItem->m_Version == CMapItemGroup_v1::CURRENT_VERSION && Size == sizeof(CMapItemGroup_v1))
		return true;
	else if(pItem->m_Version == CMapItemGroup_v2::CURRENT_VERSION && Size == sizeof(CMapItemGroup_v2))
		return true;
	else if(pItem->m_Version == CMapItemGroup_v3::CURRENT_VERSION && Size == sizeof(CMapItemGroup_v3))
		return true;
	return Size >= sizeof(CMapItemGroup_v3);
}

bool CMapItemChecker::IsValid(const CMapItemLayer *pItem, unsigned Size)
{
	// m_Version is unused
	if(!pItem || Size < sizeof(CMapItemLayer))
		return false;
	if(pItem->m_Type == LAYERTYPE_TILES)
		return IsValid(reinterpret_cast<const CMapItemLayerTilemap *>(pItem), Size);
	if(pItem->m_Type == LAYERTYPE_QUADS)
		return IsValid(reinterpret_cast<const CMapItemLayerQuads *>(pItem), Size);
	return true; // allow unknown layers types
}

bool CMapItemChecker::IsValid(const CMapItemLayerTilemap *pItem, unsigned Size)
{
	if(!pItem || Size < sizeof(CMapItemLayerTilemap_v2) || pItem->m_Version < CMapItemLayerTilemap_v2::CURRENT_VERSION
		|| pItem->m_Width < CMapItemLayerTilemap::MIN_DIMENSION || pItem->m_Height < CMapItemLayerTilemap::MIN_DIMENSION
		|| pItem->m_Width > CMapItemLayerTilemap::MAX_DIMENSION || pItem->m_Height > CMapItemLayerTilemap::MAX_DIMENSION)
		return false;
	else if(pItem->m_Version == CMapItemLayerTilemap_v2::CURRENT_VERSION && Size == sizeof(CMapItemLayerTilemap_v2))
		return true;
	else if(pItem->m_Version == CMapItemLayerTilemap_v3::CURRENT_VERSION && Size == sizeof(CMapItemLayerTilemap_v3))
		return true;
	else if(pItem->m_Version == CMapItemLayerTilemap_v4::CURRENT_VERSION && Size == sizeof(CMapItemLayerTilemap_v4))
		return true;
	return Size >= sizeof(CMapItemLayerTilemap_v4);
}

bool CMapItemChecker::IsValid(const CMapItemLayerQuads *pItem, unsigned Size)
{
	if(!pItem || Size < sizeof(CMapItemLayerQuads_v1) || pItem->m_Version < CMapItemLayerQuads_v1::CURRENT_VERSION
		|| pItem->m_NumQuads < CMapItemLayerQuads::MIN_QUADS || pItem->m_NumQuads > CMapItemLayerQuads::MAX_QUADS)
		return false;
	else if(pItem->m_Version == CMapItemLayerQuads_v1::CURRENT_VERSION && Size == sizeof(CMapItemLayerQuads_v1))
		return true;
	else if(pItem->m_Version == CMapItemLayerQuads_v2::CURRENT_VERSION && Size == sizeof(CMapItemLayerQuads_v2))
		return true;
	return Size >= sizeof(CMapItemLayerQuads_v2);
}

bool CMapItemChecker::IsValid(const CMapItemVersion *pItem, unsigned Size)
{
	if(!pItem || Size < sizeof(CMapItemVersion_v1) || pItem->m_Version < CMapItemVersion_v1::CURRENT_VERSION)
		return false;
	else if(pItem->m_Version == CMapItemVersion_v1::CURRENT_VERSION && Size == sizeof(CMapItemVersion_v1))
		return true;
	return Size >= sizeof(CMapItemVersion_v1);
}

bool CMapItemChecker::IsValid(const CEnvPoint *pItem, unsigned Size, int EnvelopeItemVersion, int *pNumPoints)
{
	if(pNumPoints)
		*pNumPoints = 0;
	if(!pItem || EnvelopeItemVersion < CMapItemEnvelope_v1::CURRENT_VERSION)
		return false;
	const int PointSize = EnvelopeItemVersion >= CMapItemEnvelope_v3::CURRENT_VERSION ? sizeof(CEnvPoint_v2) : sizeof(CEnvPoint_v1);
	if(Size % PointSize != 0)
		return false;
	if(pNumPoints)
		*pNumPoints = Size / PointSize;
	return true;
}

bool CMapItemChecker::IsValid(const CMapItemEnvelope *pItem, unsigned Size)
{
	if(!pItem || Size < sizeof(CMapItemEnvelope_v1) || pItem->m_Version < CMapItemEnvelope_v1::CURRENT_VERSION)
		return false;
	else if(pItem->m_Version == CMapItemEnvelope_v1::CURRENT_VERSION && Size == sizeof(CMapItemEnvelope_v1))
		return true;
	else if(pItem->m_Version == CMapItemEnvelope_v2::CURRENT_VERSION && Size == sizeof(CMapItemEnvelope_v2))
		return true;
	else if(pItem->m_Version == CMapItemEnvelope_v3::CURRENT_VERSION && Size == sizeof(CMapItemEnvelope_v3))
		return true;
	return Size >= sizeof(CMapItemEnvelope_v3);
}
