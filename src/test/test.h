/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef TEST_TEST_H
#define TEST_TEST_H
class CTestInfo
{
public:
	CTestInfo();
	void Filename(char *pBuffer, int BufferLength, const char *pSuffix);
	char m_aFilenamePrefix[64];
	char m_aFilename[64];
};
#endif // TEST_TEST_H
