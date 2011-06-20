/* CWebapp class by Sushi and Redix*/
#include <stdio.h>

#include <base/math.h>

#include "webapp.h"

CWebapp::CWebapp(IStorage *pStorage, const char* WebappIp)
: m_pStorage(pStorage)
{
	char aBuf[512];
	int Port = 80;
	str_copy(aBuf, WebappIp, sizeof(aBuf));

	for(int k = 0; aBuf[k]; k++)
	{
		if(aBuf[k] == ':')
		{
			Port = str_toint(aBuf+k+1);
			aBuf[k] = 0;
			break;
		}
	}

	if(net_host_lookup(aBuf, &m_Addr, NETTYPE_IPV4) != 0)
	{
		net_host_lookup("localhost", &m_Addr, NETTYPE_IPV4);
	}
	
	m_Addr.port = Port;
	
	// only one at a time
	m_JobPool.Init(1);
	m_Jobs.delete_all();
	
	m_OutputLock = lock_create();
	m_pFirst = 0;
	m_pLast = 0;
	
	m_Online = 0;
}

CWebapp::~CWebapp()
{
	// wait for the runnig jobs
	do
	{
		UpdateJobs();
	} while(m_Jobs.size() > 0);
	m_Jobs.delete_all();

	IDataOut *pNext;
	for(IDataOut *pItem = m_pFirst; pItem; pItem = pNext)
	{
		pNext = pItem->m_pNext;
		delete pItem;
	}

	lock_destroy(m_OutputLock);
}

void CWebapp::AddOutput(IDataOut *pOut)
{
	lock_wait(m_OutputLock);
	pOut->m_pNext = 0;
	if(m_pLast)
		m_pLast->m_pNext = pOut;
	else
		m_pFirst = pOut;
	m_pLast = pOut;
	lock_release(m_OutputLock);
}

bool CWebapp::Connect()
{
	// connect to the server
	m_Socket = net_tcp_create(m_Addr);
	if(m_Socket.type == NETTYPE_INVALID)
		return false;
	
	return true;
}

void CWebapp::Disconnect()
{
	net_tcp_close(m_Socket);
}

int CWebapp::GetHeaderInfo(char *pStr, int MaxSize, CHeader *pHeader)
{
	char aBuf[512] = {0};
	char *pData = pStr;
	while(str_comp_num(pData, "\r\n\r\n", 4) != 0)
	{
		pData++;
		if(pData > pStr+MaxSize)
			return -1;
	}
	pData += 4;
	int HeaderSize = pData - pStr;
	int BufSize = min((int)sizeof(aBuf),HeaderSize);
	mem_copy(aBuf, pStr, BufSize);
	
	pData = aBuf;
	//dbg_msg("webapp", "\n---header start---\n%s\n---header end---\n", aBuf);
	
	if(sscanf(pData, "HTTP/%*d.%*d %d %*s\r\n", &pHeader->m_StatusCode) != 1)
		return -2;
	
	while(sscanf(pData, "Content-Length: %ld\r\n", &pHeader->m_ContentLength) != 1)
	{
		while(str_comp_num(pData, "\r\n", 2) != 0)
		{
			pData++;
			if(pData > aBuf+BufSize)
				return -3;
		}
		pData += 2;
	}
	
	return HeaderSize;
}

int CWebapp::RecvHeader(char *pBuf, int MaxSize, CHeader *pHeader)
{
	int HeaderSize;
	int AddSize;
	int Size = 0;
	do
	{
		char *pWrite = pBuf + Size;
		AddSize = net_tcp_recv(m_Socket, pWrite, MaxSize-Size);
		Size += AddSize;
		HeaderSize = GetHeaderInfo(pBuf, Size, pHeader);
	} while(HeaderSize == -1 && MaxSize-Size > 0 && AddSize > 0);
	pHeader->m_Size = HeaderSize;
	return Size;
}

int CWebapp::SendAndReceive(const char *pInString, char **ppOutString)
{
	//dbg_msg("webapp", "\n---send start---\n%s\n---send end---\n", pInString);

	net_tcp_connect(m_Socket, &m_Addr);
	net_tcp_send(m_Socket, pInString, str_length(pInString));

	CHeader Header;
	int Size = 0;
	int MemLeft = 0;
	char *pWrite = 0;
	do
	{
		char aBuf[512] = {0};
		char *pData = aBuf;
		if(!pWrite)
		{
			Size = RecvHeader(aBuf, sizeof(aBuf), &Header);

			if(Header.m_Size < 0)
				return -1;

			if(Header.m_StatusCode != 200)
				return -Header.m_StatusCode;

			pData += Header.m_Size;
			MemLeft = Header.m_ContentLength;
			*ppOutString = (char *)mem_alloc(MemLeft+1, 1);
			mem_zero(*ppOutString, MemLeft+1);
			pWrite = *ppOutString;
		}
		else
			Size = net_tcp_recv(m_Socket, aBuf, sizeof(aBuf));

		if(Size > 0)
		{
			int Write = Size - (pData - aBuf);
			if(Write > MemLeft)
			{
				mem_free(*ppOutString);
				return -2;
			}
			mem_copy(pWrite, pData, Write);
			pWrite += Write;
			MemLeft = *ppOutString + Header.m_ContentLength - pWrite;
		}
	} while(Size > 0);

	if(MemLeft != 0)
	{
		mem_free(*ppOutString);
		return -3;
	}

	//dbg_msg("webapp", "\n---recv start---\n%s\n---recv end---\n", *ppOutString);

	return Header.m_ContentLength;
}

CJob *CWebapp::AddJob(JOBFUNC pfnFunc, IDataIn *pUserData, bool NeedOnline)
{
	if(NeedOnline && !m_Online)
	{
		delete pUserData;
		return 0;
	}

	pUserData->m_pWebapp = this;
	int i = m_Jobs.add(new CJob());
	m_JobPool.Add(m_Jobs[i], pfnFunc, pUserData);
	return m_Jobs[i];
}

int CWebapp::UpdateJobs()
{
	int Num = 0;
	for(int i = 0; i < m_Jobs.size(); i++)
	{
		if(m_Jobs[i]->Status() == CJob::STATE_DONE)
		{
			delete m_Jobs[i];
			m_Jobs.remove_index_fast(i);
			Num++;
		}
	}
	return Num;
}
