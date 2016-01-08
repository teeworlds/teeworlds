#include <modapi/server/editorcreator.h>
#include <modapi/graphics.h>

#include <engine/storage.h>
#include <engine/shared/datafile.h>
#include <engine/graphics.h>

#include <engine/external/pnglite/pnglite.h>

CModAPI_EditorRessourcesCreator::CModAPI_EditorRessourcesCreator(const char* pName)
{
	str_copy(m_aName, pName, sizeof(m_aName));
}

int CModAPI_EditorRessourcesCreator::AddImage(IStorage* pStorage, const char* pFilename)
{
	CModAPI_EditorItem_Image Image;
	
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention

	// open file for reading
	png_init(0,0); // ignore_convention

	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL, aCompleteFilename, sizeof(aCompleteFilename));
	if(File)
	{
		io_close(File);
	}
	else
	{
		dbg_msg("editorressources", "failed to open file. filename='%s'", pFilename);
		return -1;
	}

	int Error = png_open_file(&Png, aCompleteFilename); // ignore_convention
	if(Error != PNG_NO_ERROR)
	{
		dbg_msg("editorressources", "failed to open file. filename='%s'", aCompleteFilename);
		if(Error != PNG_FILE_ERROR)
		{
			png_close_file(&Png); // ignore_convention
		}
		return -1;
	}

	if(Png.depth != 8 || (Png.color_type != PNG_TRUECOLOR && Png.color_type != PNG_TRUECOLOR_ALPHA) || Png.width > (2<<12) || Png.height > (2<<12)) // ignore_convention
	{
		dbg_msg("editorressources", "invalid format. filename='%s'", aCompleteFilename);
		png_close_file(&Png); // ignore_convention
		return -1;
	}

	pBuffer = (unsigned char *)mem_alloc(Png.width * Png.height * Png.bpp, 1); // ignore_convention
	png_get_data(&Png, pBuffer); // ignore_convention
	png_close_file(&Png); // ignore_convention

	Image.m_Id = m_Images.size();
	Image.m_Width = Png.width; // ignore_convention
	Image.m_Height = Png.height; // ignore_convention
	if(Png.color_type == PNG_TRUECOLOR) // ignore_convention
		Image.m_Format = CImageInfo::FORMAT_RGB;
	else if(Png.color_type == PNG_TRUECOLOR_ALPHA) // ignore_convention
		Image.m_Format = CImageInfo::FORMAT_RGBA;
	
	m_Images.add(Image);
	m_ImagesData.add(pBuffer);
	
	return Image.m_Id;
}

CModAPI_EditorRessourcesCreator::CEntityPointTypeCreator& CModAPI_EditorRessourcesCreator::AddEntityPointType(int Id, const char* pName)
{
	if(Id < 0 || Id > 255)
	{
		dbg_msg("editorressources", "wrong entity point type id: %d", Id);
	}
	
	m_EntityPoints.set_size(m_EntityPoints.size()+1);
	CEntityPointTypeCreator& EntityPointType = m_EntityPoints[m_EntityPoints.size()-1];
	
	EntityPointType.m_Id = Id;
	str_copy(EntityPointType.m_aNameData, pName, sizeof(EntityPointType.m_aNameData));
	
	return EntityPointType;
}

int CModAPI_EditorRessourcesCreator::Save(class IStorage *pStorage, const char *pFileName)
{
	CDataFileWriter df;
	if(!df.Open(pStorage, pFileName))
	{
		dbg_msg("editorressources", "can't create the editor file %s", pFileName);
		return 0;
	}
	
	CModAPI_EditorItem_Info Info;
	Info.m_Name = df.AddData(str_length(m_aName)+1, m_aName);
	df.AddItem(MODAPI_EDITORITEMTYPE_INFO, 0, sizeof(CModAPI_EditorItem_Info), &Info);
	
	//Save images
	for(int i=0; i<m_Images.size(); i++)
	{
		CModAPI_EditorItem_Image* pImage = &m_Images[i];
		int PixelSize = pImage->m_Format == CImageInfo::FORMAT_RGB ? 3 : 4;
		pImage->m_ImageData = df.AddData(pImage->m_Width*pImage->m_Height*PixelSize, m_ImagesData[i]);
		df.AddItem(MODAPI_EDITORITEMTYPE_IMAGE, i, sizeof(CModAPI_EditorItem_Image), pImage);
	}
	
	//Save entity point types
	for(int i=0; i<m_EntityPoints.size(); i++)
	{
		CEntityPointTypeCreator* pEntityPointType = &m_EntityPoints[i];
		pEntityPointType->m_Name = df.AddData(str_length(pEntityPointType->m_aNameData)+1, pEntityPointType->m_aNameData);
		df.AddItem(MODAPI_EDITORITEMTYPE_ENTITYPOINTTYPE, i, sizeof(CModAPI_EditorItem_EntityPointType), pEntityPointType);
	}
	
	df.Finish();
	
	return 1;
}
