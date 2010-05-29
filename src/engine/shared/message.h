#ifndef ENGINE_SHARED_MESSAGE_H
#define ENGINE_SHARED_MESSAGE_H
class CMessage
{
public:
	virtual bool Pack(void *pData, unsigned MaxDataSize);
	virtual bool Unpack(const void *pData, unsigned DataSize);
};
#endif
