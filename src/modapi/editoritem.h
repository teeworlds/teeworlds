#ifndef MODAPI_EDITORITEM_H
#define MODAPI_EDITORITEM_H

enum
{
	MODAPI_EDITORITEMTYPE_INFO = 0,
	MODAPI_EDITORITEMTYPE_IMAGE,
	MODAPI_EDITORITEMTYPE_ENTITYPOINTTYPE,
};

struct CModAPI_EditorItem_Info
{
	int m_Name;
};

struct CModAPI_EditorItem_Image
{
	int m_Id;
	int m_Width;
	int m_Height;
	int m_Format;
	int m_ImageData;
};

struct CModAPI_EditorItem_EntityPointType
{
	int m_Id;
	int m_Name;
};

#endif
