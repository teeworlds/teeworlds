/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/math.h>

#include <engine/storage.h>

#include "filecollection.h"

bool CFileCollection::IsFilenameValid(const char *pFilename)
{
	if(m_aFileDesc[0] == '\0')
	{
		int FilenameLength = str_length(pFilename);
		if(m_FileExtLength+TIMESTAMP_LENGTH > FilenameLength)
		{
			return false;
		}

		pFilename += FilenameLength-m_FileExtLength-TIMESTAMP_LENGTH;
	}
	else
	{
		if(str_length(pFilename) != m_FileDescLength+TIMESTAMP_LENGTH+m_FileExtLength ||
			str_comp_num(pFilename, m_aFileDesc, m_FileDescLength) ||
			str_comp(pFilename+m_FileDescLength+TIMESTAMP_LENGTH, m_aFileExt))
			return false;

		pFilename += m_FileDescLength;
	}

	if(pFilename[0] == '_' &&
		pFilename[1] >= '0' && pFilename[1] <= '9' &&
		pFilename[2] >= '0' && pFilename[2] <= '9' &&
		pFilename[3] >= '0' && pFilename[3] <= '9' &&
		pFilename[4] >= '0' && pFilename[4] <= '9' &&
		pFilename[5] == '-' &&
		pFilename[6] >= '0' && pFilename[6] <= '9' &&
		pFilename[7] >= '0' && pFilename[7] <= '9' &&
		pFilename[8] == '-' &&
		pFilename[9] >= '0' && pFilename[9] <= '9' &&
		pFilename[10] >= '0' && pFilename[10] <= '9' &&
		pFilename[11] == '_' &&
		pFilename[12] >= '0' && pFilename[12] <= '9' &&
		pFilename[13] >= '0' && pFilename[13] <= '9' &&
		pFilename[14] == '-' &&
		pFilename[15] >= '0' && pFilename[15] <= '9' &&
		pFilename[16] >= '0' && pFilename[16] <= '9' &&
		pFilename[17] == '-' &&
		pFilename[18] >= '0' && pFilename[18] <= '9' &&
		pFilename[19] >= '0' && pFilename[19] <= '9')
		return true;

	return false;
}

int64 CFileCollection::ExtractTimestamp(const char *pTimestring)
{
	int64 Timestamp = pTimestring[0]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[1]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[2]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[3]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[5]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[6]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[8]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[9]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[11]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[12]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[14]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[15]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[17]-'0'; Timestamp <<= 4;
	Timestamp += pTimestring[18]-'0';

	return Timestamp;
}

void CFileCollection::BuildTimestring(int64 Timestamp, char *pTimestring)
{
	pTimestring[19] = 0;
	pTimestring[18] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[17] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[16] = '-';
	pTimestring[15] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[14] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[13] = '-';
	pTimestring[12] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[11] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[10] = '_';
	pTimestring[9] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[8] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[7] = '-';
	pTimestring[6] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[5] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[4] = '-';
	pTimestring[3] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[2] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[1] = (Timestamp&0xF)+'0'; Timestamp >>= 4;
	pTimestring[0] = (Timestamp&0xF)+'0';
}

void CFileCollection::Init(IStorage *pStorage, const char *pPath, const char *pFileDesc, const char *pFileExt, int MaxEntries)
{
	mem_zero(m_aTimestamps, sizeof(m_aTimestamps));
	m_NumTimestamps = 0;
	m_Remove = -1;
	m_MaxEntries = clamp(MaxEntries, 1, static_cast<int>(MAX_ENTRIES));
	str_copy(m_aFileDesc, pFileDesc, sizeof(m_aFileDesc));
	m_FileDescLength = str_length(m_aFileDesc);
	str_copy(m_aFileExt, pFileExt, sizeof(m_aFileExt));
	m_FileExtLength = str_length(m_aFileExt);
	str_copy(m_aPath, pPath, sizeof(m_aPath));
	m_pStorage = pStorage;

	m_pStorage->ListDirectory(IStorage::TYPE_SAVE, m_aPath, FilelistCallback, this);
}

void CFileCollection::AddEntry(int64 Timestamp)
{
	if(m_NumTimestamps == 0)
	{
		// empty list
		m_aTimestamps[m_NumTimestamps++] = Timestamp;
	}
	else
	{
		// remove old file
		if(m_NumTimestamps >= m_MaxEntries)
		{
			if(m_aFileDesc[0] == '\0') // consider an empty file desc as a wild card
			{
				m_Remove = m_aTimestamps[0];
				m_pStorage->ListDirectory(IStorage::TYPE_SAVE, m_aPath, RemoveCallback, this);
			}
			else
			{
				char aBuf[512];
				char aTimestring[TIMESTAMP_LENGTH];
				BuildTimestring(m_aTimestamps[0], aTimestring);

				str_format(aBuf, sizeof(aBuf), "%s/%s_%s%s", m_aPath, m_aFileDesc, aTimestring, m_aFileExt);
				m_pStorage->RemoveFile(aBuf, IStorage::TYPE_SAVE);
			}
		}

		// add entry to the sorted list
		if(m_aTimestamps[0] > Timestamp)
		{
			// first entry
			if(m_NumTimestamps < m_MaxEntries)
			{
				mem_move(m_aTimestamps+1, m_aTimestamps, m_NumTimestamps*sizeof(int64));
				m_aTimestamps[0] = Timestamp;
				++m_NumTimestamps;
			}
		}
		else if(m_aTimestamps[m_NumTimestamps-1] <= Timestamp)
		{
			// last entry
			if(m_NumTimestamps == m_MaxEntries)
			{
				mem_move(m_aTimestamps, m_aTimestamps+1, (m_NumTimestamps-1)*sizeof(int64));
				m_aTimestamps[m_NumTimestamps-1] = Timestamp;
			}
			else
				m_aTimestamps[m_NumTimestamps++] = Timestamp;
		}
		else
		{
			// middle entry
			int Left = 0, Right = m_NumTimestamps-1;
			while(Right-Left > 1)
			{
				int Mid = (Left+Right)/2;
				if(m_aTimestamps[Mid] > Timestamp)
					Right = Mid;
				else
					Left = Mid;
			}

			if(m_NumTimestamps == m_MaxEntries)
			{
				mem_move(m_aTimestamps, m_aTimestamps+1, (Right-1)*sizeof(int64));
				m_aTimestamps[Right-1] = Timestamp;
			}
			else
			{
				mem_move(m_aTimestamps+Right+1, m_aTimestamps+Right, (m_NumTimestamps-Right)*sizeof(int64));
				m_aTimestamps[Right] = Timestamp;
				++m_NumTimestamps;
			}
		}
	}
}

int64 CFileCollection::GetTimestamp(const char *pFilename)
{
	if(m_aFileDesc[0] == '\0')
	{
		int FilenameLength = str_length(pFilename);
		return ExtractTimestamp(pFilename+FilenameLength-m_FileExtLength-TIMESTAMP_LENGTH);
	}
	else
	{
		return ExtractTimestamp(pFilename+m_FileDescLength+1);
	}
}

int CFileCollection::FilelistCallback(const char *pFilename, int IsDir, int StorageType, void *pUser)
{
	CFileCollection *pThis = static_cast<CFileCollection *>(pUser);

	// check for valid file name format
	if(IsDir || !pThis->IsFilenameValid(pFilename))
		return 0;

	// extract the timestamp
	int64 Timestamp = pThis->GetTimestamp(pFilename);

	// add the entry
	pThis->AddEntry(Timestamp);

	return 0;
}

int CFileCollection::RemoveCallback(const char *pFilename, int IsDir, int StorageType, void *pUser)
{
	CFileCollection *pThis = static_cast<CFileCollection *>(pUser);

	// check for valid file name format
	if(IsDir || !pThis->IsFilenameValid(pFilename))
		return 0;

	// extract the timestamp
	int64 Timestamp = pThis->GetTimestamp(pFilename);

	if(Timestamp == pThis->m_Remove)
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%s/%s", pThis->m_aPath, pFilename);
		pThis->m_pStorage->RemoveFile(aBuf, IStorage::TYPE_SAVE);
		pThis->m_Remove = -1;
		return 1;
	}

	return 0;
}
