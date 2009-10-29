

class CMessage
{
public:
	virtual bool Pack(void *pData, unsigned MaxDataSize);
	virtual bool Unpack(const void *pData, unsigned DataSize);
};
