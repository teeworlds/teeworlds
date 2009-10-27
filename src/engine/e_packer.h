/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

class CPacker
{
	enum
	{
		PACKER_BUFFER_SIZE=1024*2
	};

	unsigned char m_aBuffer[PACKER_BUFFER_SIZE];
	unsigned char *m_pCurrent;
	unsigned char *m_pEnd;
	int m_Error;
public:
	void Reset();
	void AddInt(int i);
	void AddString(const char *pStr, int Limit);
	void AddRaw(const unsigned char *pData, int Size);
	
	int Size() const { return (int)(m_pCurrent-m_aBuffer); }
	const unsigned char *Data() const { return m_aBuffer; }
	bool Error() const { return m_Error; }
};

class CUnpacker
{
	const unsigned char *m_pStart;
	const unsigned char *m_pCurrent;
	const unsigned char *m_pEnd;
	int m_Error;
public:
	void Reset(const unsigned char *pData, int Size);
	int GetInt();
	const char *GetString();
	const unsigned char *GetRaw(int Size);
	bool Error() const { return m_Error; }
};
