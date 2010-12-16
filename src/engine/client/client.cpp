/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <stdlib.h> // qsort
#include <stdarg.h>

#include <base/math.h>
#include <base/system.h>
#include <engine/shared/engine.h>

#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>
#include <engine/shared/compression.h>
#include <engine/shared/network.h>
#include <engine/shared/config.h>
#include <engine/shared/packer.h>
#include <engine/shared/memheap.h>
#include <engine/shared/datafile.h>
#include <engine/shared/ringbuffer.h>
#include <engine/shared/protocol.h>

#include <engine/shared/demo.h>

#include <mastersrv/mastersrv.h>
#include <versionsrv/versionsrv.h>

#include "client.h"

#if defined(CONF_FAMILY_WINDOWS)
	#define _WIN32_WINNT 0x0500
	#define NOGDI
	#include <windows.h>
#endif


void CGraph::Init(float Min, float Max)
{
	m_Min = Min;
	m_Max = Max;
	m_Index = 0;
}

void CGraph::ScaleMax()
{
	int i = 0;
	m_Max = 0;
	for(i = 0; i < MAX_VALUES; i++)
	{
		if(m_aValues[i] > m_Max)
			m_Max = m_aValues[i];
	}
}

void CGraph::ScaleMin()
{
	int i = 0;
	m_Min = m_Max;
	for(i = 0; i < MAX_VALUES; i++)
	{
		if(m_aValues[i] < m_Min)
			m_Min = m_aValues[i];
	}
}

void CGraph::Add(float v, float r, float g, float b)
{
	m_Index = (m_Index+1)&(MAX_VALUES-1);
	m_aValues[m_Index] = v;
	m_aColors[m_Index][0] = r;
	m_aColors[m_Index][1] = g;
	m_aColors[m_Index][2] = b;
}

void CGraph::Render(IGraphics *pGraphics, int Font, float x, float y, float w, float h, const char *pDescription)
{
	//m_pGraphics->BlendNormal();


	pGraphics->TextureSet(-1);

	pGraphics->QuadsBegin();
	pGraphics->SetColor(0, 0, 0, 0.75f);
	IGraphics::CQuadItem QuadItem(x, y, w, h);
	pGraphics->QuadsDrawTL(&QuadItem, 1);
	pGraphics->QuadsEnd();

	pGraphics->LinesBegin();
	pGraphics->SetColor(0.95f, 0.95f, 0.95f, 1.00f);
	IGraphics::CLineItem LineItem(x, y+h/2, x+w, y+h/2);
	pGraphics->LinesDraw(&LineItem, 1);
	pGraphics->SetColor(0.5f, 0.5f, 0.5f, 0.75f);
	IGraphics::CLineItem Array[2] = {
		IGraphics::CLineItem(x, y+(h*3)/4, x+w, y+(h*3)/4),
		IGraphics::CLineItem(x, y+h/4, x+w, y+h/4)};
	pGraphics->LinesDraw(Array, 2);
	for(int i = 1; i < MAX_VALUES; i++)
	{
		float a0 = (i-1)/(float)MAX_VALUES;
		float a1 = i/(float)MAX_VALUES;
		int i0 = (m_Index+i-1)&(MAX_VALUES-1);
		int i1 = (m_Index+i)&(MAX_VALUES-1);

		float v0 = (m_aValues[i0]-m_Min) / (m_Max-m_Min);
		float v1 = (m_aValues[i1]-m_Min) / (m_Max-m_Min);

		IGraphics::CColorVertex Array[2] = {
			IGraphics::CColorVertex(0, m_aColors[i0][0], m_aColors[i0][1], m_aColors[i0][2], 0.75f),
			IGraphics::CColorVertex(1, m_aColors[i1][0], m_aColors[i1][1], m_aColors[i1][2], 0.75f)};
		pGraphics->SetColorVertex(Array, 2);
		IGraphics::CLineItem LineItem(x+a0*w, y+h-v0*h, x+a1*w, y+h-v1*h);
		pGraphics->LinesDraw(&LineItem, 1);

	}
	pGraphics->LinesEnd();

	pGraphics->TextureSet(Font);
	pGraphics->QuadsText(x+2, y+h-16, 16, 1,1,1,1, pDescription);

	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%.2f", m_Max);
	pGraphics->QuadsText(x+w-8*str_length(aBuf)-8, y+2, 16, 1,1,1,1, aBuf);

	str_format(aBuf, sizeof(aBuf), "%.2f", m_Min);
	pGraphics->QuadsText(x+w-8*str_length(aBuf)-8, y+h-16, 16, 1,1,1,1, aBuf);
}


void CSmoothTime::Init(int64 Target)
{
	m_Snap = time_get();
	m_Current = Target;
	m_Target = Target;
	m_aAdjustSpeed[0] = 0.3f;
	m_aAdjustSpeed[1] = 0.3f;
	m_Graph.Init(0.0f, 0.5f);
}

void CSmoothTime::SetAdjustSpeed(int Direction, float Value)
{
	m_aAdjustSpeed[Direction] = Value;
}

int64 CSmoothTime::Get(int64 Now)
{
	int64 c = m_Current + (Now - m_Snap);
	int64 t = m_Target + (Now - m_Snap);

	// it's faster to adjust upward instead of downward
	// we might need to adjust these abit

	float AdjustSpeed = m_aAdjustSpeed[0];
	if(t > c)
		AdjustSpeed = m_aAdjustSpeed[1];

	float a = ((Now-m_Snap)/(float)time_freq()) * AdjustSpeed;
	if(a > 1.0f)
		a = 1.0f;

	int64 r = c + (int64)((t-c)*a);

	m_Graph.Add(a+0.5f,1,1,1);

	return r;
}

void CSmoothTime::UpdateInt(int64 Target)
{
	int64 Now = time_get();
	m_Current = Get(Now);
	m_Snap = Now;
	m_Target = Target;
}

void CSmoothTime::Update(CGraph *pGraph, int64 Target, int TimeLeft, int AdjustDirection)
{
	int UpdateTimer = 1;

	if(TimeLeft < 0)
	{
		int IsSpike = 0;
		if(TimeLeft < -50)
		{
			IsSpike = 1;

			m_SpikeCounter += 5;
			if(m_SpikeCounter > 50)
				m_SpikeCounter = 50;
		}

		if(IsSpike && m_SpikeCounter < 15)
		{
			// ignore this ping spike
			UpdateTimer = 0;
			pGraph->Add(TimeLeft, 1,1,0);
		}
		else
		{
			pGraph->Add(TimeLeft, 1,0,0);
			if(m_aAdjustSpeed[AdjustDirection] < 30.0f)
				m_aAdjustSpeed[AdjustDirection] *= 2.0f;
		}
	}
	else
	{
		if(m_SpikeCounter)
			m_SpikeCounter--;

		pGraph->Add(TimeLeft, 0,1,0);

		m_aAdjustSpeed[AdjustDirection] *= 0.95f;
		if(m_aAdjustSpeed[AdjustDirection] < 2.0f)
			m_aAdjustSpeed[AdjustDirection] = 2.0f;
	}

	if(UpdateTimer)
		UpdateInt(Target);
}


bool CFileCollection::IsFilenameValid(const char *pFilename)
{
	if(str_length(pFilename) != m_FileDescLength+TIMESTAMP_LENGTH+m_FileExtLength ||
		str_comp_num(pFilename, m_aFileDesc, m_FileDescLength) ||
		str_comp(pFilename+m_FileDescLength+TIMESTAMP_LENGTH, m_aFileExt))
		return false;

	pFilename += m_FileDescLength;
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
		if(m_NumTimestamps == m_MaxEntries)
		{
			char aBuf[512];
			char aTimestring[TIMESTAMP_LENGTH];
			BuildTimestring(m_aTimestamps[0], aTimestring);
			str_format(aBuf, sizeof(aBuf), "%s/%s_%s%s", m_aPath, m_aFileDesc, aTimestring, m_aFileExt);
			m_pStorage->RemoveFile(aBuf, IStorage::TYPE_SAVE);
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

void CFileCollection::FilelistCallback(const char *pFilename, int IsDir, int StorageType, void *pUser)
{
	CFileCollection *pThis = static_cast<CFileCollection *>(pUser);

	// check for valid file name format
	if(IsDir || !pThis->IsFilenameValid(pFilename))
		return;

	// extract the timestamp
	int64 Timestamp = pThis->ExtractTimestamp(pFilename+pThis->m_FileDescLength+1);

	// add the entry
	pThis->AddEntry(Timestamp);
}


CClient::CClient() : m_DemoPlayer(&m_SnapshotDelta), m_DemoRecorder(&m_SnapshotDelta)
{
	m_pEditor = 0;
	m_pInput = 0;
	m_pGraphics = 0;
	m_pSound = 0;
	m_pGameClient = 0;
	m_pMap = 0;
	m_pConsole = 0;

	m_FrameTime = 0.0001f;
	m_FrameTimeLow = 1.0f;
	m_FrameTimeHigh = 0.0f;
	m_Frames = 0;

	m_GameTickSpeed = SERVER_TICK_SPEED;

	m_WindowMustRefocus = 0;
	m_SnapCrcErrors = 0;
	m_AutoScreenshotRecycle = false;

	m_AckGameTick = -1;
	m_CurrentRecvTick = 0;
	m_RconAuthed = 0;

	// version-checking
	m_aVersionStr[0] = '0';
	m_aVersionStr[1] = 0;

	// pinging
	m_PingStartTime = 0;

	//
	m_aCurrentMap[0] = 0;
	m_CurrentMapCrc = 0;

	//
	m_aCmdConnect[0] = 0;

	// map download
	m_aMapdownloadFilename[0] = 0;
	m_aMapdownloadName[0] = 0;
	m_MapdownloadFile = 0;
	m_MapdownloadChunk = 0;
	m_MapdownloadCrc = 0;
	m_MapdownloadAmount = -1;
	m_MapdownloadTotalsize = -1;

	m_CurrentServerInfoRequestTime = -1;

	m_CurrentInput = 0;

	m_State = IClient::STATE_OFFLINE;
	m_aServerAddressStr[0] = 0;

	mem_zero(m_aSnapshots, sizeof(m_aSnapshots));
	m_RecivedSnapshots = 0;

	m_VersionInfo.m_State = 0;
}

// ----- send functions -----
int CClient::SendMsg(CMsgPacker *pMsg, int Flags)
{
	return SendMsgEx(pMsg, Flags, false);
}

int CClient::SendMsgEx(CMsgPacker *pMsg, int Flags, bool System)
{
	CNetChunk Packet;

	if(State() == IClient::STATE_OFFLINE)
		return 0;

	mem_zero(&Packet, sizeof(CNetChunk));

	Packet.m_ClientID = 0;
	Packet.m_pData = pMsg->Data();
	Packet.m_DataSize = pMsg->Size();

	// HACK: modify the message id in the packet and store the system flag
	if(*((unsigned char*)Packet.m_pData) == 1 && System && Packet.m_DataSize == 1)
		dbg_break();

	*((unsigned char*)Packet.m_pData) <<= 1;
	if(System)
		*((unsigned char*)Packet.m_pData) |= 1;

	if(Flags&MSGFLAG_VITAL)
		Packet.m_Flags |= NETSENDFLAG_VITAL;
	if(Flags&MSGFLAG_FLUSH)
		Packet.m_Flags |= NETSENDFLAG_FLUSH;

	if(Flags&MSGFLAG_RECORD)
	{
		if(m_DemoRecorder.IsRecording())
			m_DemoRecorder.RecordMessage(Packet.m_pData, Packet.m_DataSize);
	}

	if(!(Flags&MSGFLAG_NOSEND))
		m_NetClient.Send(&Packet);
	return 0;
}

void CClient::SendInfo()
{
	CMsgPacker Msg(NETMSG_INFO);
	Msg.AddString(GameClient()->NetVersion(), 128);
	Msg.AddString(g_Config.m_PlayerName, 128);
	Msg.AddString(g_Config.m_ClanName, 128);
	Msg.AddString(g_Config.m_Password, 128);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}


void CClient::SendEnterGame()
{
	CMsgPacker Msg(NETMSG_ENTERGAME);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}

void CClient::SendReady()
{
	CMsgPacker Msg(NETMSG_READY);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}

bool CClient::RconAuthed()
{
	return m_RconAuthed;
}

void CClient::RconAuth(const char *pName, const char *pPassword)
{
	if(RconAuthed())
		return;
        
	CMsgPacker Msg(NETMSG_RCON_AUTH);
	Msg.AddString(pName, 32);
	Msg.AddString(pPassword, 32);
	SendMsgEx(&Msg, MSGFLAG_VITAL);
}

void CClient::Rcon(const char *pCmd)
{
	CMsgPacker Msg(NETMSG_RCON_CMD);
	Msg.AddString(pCmd, 256);
	SendMsgEx(&Msg, MSGFLAG_VITAL);
}

bool CClient::ConnectionProblems()
{
	return m_NetClient.GotProblems() != 0;
}

void CClient::DirectInput(int *pInput, int Size)
{
	int i;
	CMsgPacker Msg(NETMSG_INPUT);
	Msg.AddInt(m_AckGameTick);
	Msg.AddInt(m_PredTick);
	Msg.AddInt(Size);

	for(i = 0; i < Size/4; i++)
		Msg.AddInt(pInput[i]);

	SendMsgEx(&Msg, 0);
}


void CClient::SendInput()
{
	int64 Now = time_get();

	if(m_PredTick <= 0)
		return;

	// fetch input
	int Size = GameClient()->OnSnapInput(m_aInputs[m_CurrentInput].m_aData);

	if(!Size)
		return;

	// pack input
	CMsgPacker Msg(NETMSG_INPUT);
	Msg.AddInt(m_AckGameTick);
	Msg.AddInt(m_PredTick);
	Msg.AddInt(Size);

	m_aInputs[m_CurrentInput].m_Tick = m_PredTick;
	m_aInputs[m_CurrentInput].m_PredictedTime = m_PredictedTime.Get(Now);
	m_aInputs[m_CurrentInput].m_Time = Now;

	// pack it
	for(int i = 0; i < Size/4; i++)
		Msg.AddInt(m_aInputs[m_CurrentInput].m_aData[i]);

	m_CurrentInput++;
	m_CurrentInput%=200;

	SendMsgEx(&Msg, MSGFLAG_FLUSH);
}

const char *CClient::LatestVersion()
{
	return m_aVersionStr;
}

// TODO: OPT: do this alot smarter!
int *CClient::GetInput(int Tick)
{
	int Best = -1;
	for(int i = 0; i < 200; i++)
	{
		if(m_aInputs[i].m_Tick <= Tick && (Best == -1 || m_aInputs[Best].m_Tick < m_aInputs[i].m_Tick))
			Best = i;
	}

	if(Best != -1)
		return (int *)m_aInputs[Best].m_aData;
	return 0;
}

// ------ state handling -----
void CClient::SetState(int s)
{
	int Old = m_State;
	if(g_Config.m_Debug)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "state change. last=%d current=%d", m_State, s);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
	}
	m_State = s;
	if(Old != s)
		GameClient()->OnStateChange(m_State, Old);
}

// called when the map is loaded and we should init for a new round
void CClient::OnEnterGame()
{
	// reset input
	int i;
	for(i = 0; i < 200; i++)
		m_aInputs[i].m_Tick = -1;
	m_CurrentInput = 0;

	// reset snapshots
	m_aSnapshots[SNAP_CURRENT] = 0;
	m_aSnapshots[SNAP_PREV] = 0;
	m_SnapshotStorage.PurgeAll();
	m_RecivedSnapshots = 0;
	m_SnapshotParts = 0;
	m_PredTick = 0;
	m_CurrentRecvTick = 0;
	m_CurGameTick = 0;
	m_PrevGameTick = 0;
}

void CClient::EnterGame()
{
	if(State() == IClient::STATE_DEMOPLAYBACK)
		return;

	// now we will wait for two snapshots
	// to finish the connection
	SendEnterGame();
	OnEnterGame();
}

void CClient::Connect(const char *pAddress)
{
	char aBuf[512];
	int Port = 8303;

	Disconnect();

	str_copy(m_aServerAddressStr, pAddress, sizeof(m_aServerAddressStr));

	str_format(aBuf, sizeof(aBuf), "connecting to '%s'", m_aServerAddressStr);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);

	ServerInfoRequest();
	str_copy(aBuf, m_aServerAddressStr, sizeof(aBuf));

	for(int k = 0; aBuf[k]; k++)
	{
		if(aBuf[k] == ':')
		{
			Port = str_toint(aBuf+k+1);
			aBuf[k] = 0;
			break;
		}
	}

	// TODO: IPv6 support
	if(net_host_lookup(aBuf, &m_ServerAddress, NETTYPE_IPV4) != 0)
	{
		char aBufMsg[256];
		str_format(aBufMsg, sizeof(aBufMsg), "could not find the address of %s, connecting to localhost", aBuf);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBufMsg);
		net_host_lookup("localhost", &m_ServerAddress, NETTYPE_IPV4);
	}

	m_RconAuthed = 0;
	m_ServerAddress.port = Port;
	m_NetClient.Connect(&m_ServerAddress);
	SetState(IClient::STATE_CONNECTING);

	if(m_DemoRecorder.IsRecording())
		DemoRecorder_Stop();

	m_InputtimeMarginGraph.Init(-150.0f, 150.0f);
	m_GametimeMarginGraph.Init(-150.0f, 150.0f);
}

void CClient::DisconnectWithReason(const char *pReason)
{
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "disconnecting. reason='%s'", pReason?pReason:"unknown");
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);

	// stop demo playback and recorder
	m_DemoPlayer.Stop();
	DemoRecorder_Stop();

	//
	m_RconAuthed = 0;
	m_NetClient.Disconnect(pReason);
	SetState(IClient::STATE_OFFLINE);
	m_pMap->Unload();

	// disable all downloads
	m_MapdownloadChunk = 0;
	if(m_MapdownloadFile)
		io_close(m_MapdownloadFile);
	m_MapdownloadFile = 0;
	m_MapdownloadCrc = 0;
	m_MapdownloadTotalsize = -1;
	m_MapdownloadAmount = 0;

	// clear the current server info
	mem_zero(&m_CurrentServerInfo, sizeof(m_CurrentServerInfo));
	mem_zero(&m_ServerAddress, sizeof(m_ServerAddress));

	// clear snapshots
	m_aSnapshots[SNAP_CURRENT] = 0;
	m_aSnapshots[SNAP_PREV] = 0;
	m_RecivedSnapshots = 0;
}

void CClient::Disconnect()
{
	DisconnectWithReason(0);
}


void CClient::GetServerInfo(CServerInfo *pServerInfo)
{
	mem_copy(pServerInfo, &m_CurrentServerInfo, sizeof(m_CurrentServerInfo));
}

void CClient::ServerInfoRequest()
{
	mem_zero(&m_CurrentServerInfo, sizeof(m_CurrentServerInfo));
	m_CurrentServerInfoRequestTime = 0;
}

int CClient::LoadData()
{
	m_DebugFont = Graphics()->LoadTexture("debug_font.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_NORESAMPLE);
	return 1;
}

// ---

void *CClient::SnapGetItem(int SnapId, int Index, CSnapItem *pItem)
{
	CSnapshotItem *i;
	dbg_assert(SnapId >= 0 && SnapId < NUM_SNAPSHOT_TYPES, "invalid SnapId");
	i = m_aSnapshots[SnapId]->m_pAltSnap->GetItem(Index);
	pItem->m_DataSize = m_aSnapshots[SnapId]->m_pAltSnap->GetItemSize(Index);
	pItem->m_Type = i->Type();
	pItem->m_Id = i->ID();
	return (void *)i->Data();
}

void CClient::SnapInvalidateItem(int SnapId, int Index)
{
	CSnapshotItem *i;
	dbg_assert(SnapId >= 0 && SnapId < NUM_SNAPSHOT_TYPES, "invalid SnapId");
	i = m_aSnapshots[SnapId]->m_pAltSnap->GetItem(Index);
	if(i)
	{
		if((char *)i < (char *)m_aSnapshots[SnapId]->m_pAltSnap || (char *)i > (char *)m_aSnapshots[SnapId]->m_pAltSnap + m_aSnapshots[SnapId]->m_SnapSize)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "snap invalidate problem");
		if((char *)i >= (char *)m_aSnapshots[SnapId]->m_pSnap && (char *)i < (char *)m_aSnapshots[SnapId]->m_pSnap + m_aSnapshots[SnapId]->m_SnapSize)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "snap invalidate problem");
		i->m_TypeAndID = -1;
	}
}

void *CClient::SnapFindItem(int SnapId, int Type, int Id)
{
	// TODO: linear search. should be fixed.
	int i;

	if(!m_aSnapshots[SnapId])
		return 0x0;

	for(i = 0; i < m_aSnapshots[SnapId]->m_pSnap->NumItems(); i++)
	{
		CSnapshotItem *pItem = m_aSnapshots[SnapId]->m_pAltSnap->GetItem(i);
		if(pItem->Type() == Type && pItem->ID() == Id)
			return (void *)pItem->Data();
	}
	return 0x0;
}

int CClient::SnapNumItems(int SnapId)
{
	dbg_assert(SnapId >= 0 && SnapId < NUM_SNAPSHOT_TYPES, "invalid SnapId");
	if(!m_aSnapshots[SnapId])
		return 0;
	return m_aSnapshots[SnapId]->m_pSnap->NumItems();
}

void CClient::SnapSetStaticsize(int ItemType, int Size)
{
	m_SnapshotDelta.SetStaticsize(ItemType, Size);
}


void CClient::DebugRender()
{
	static NETSTATS Prev, Current;
	static int64 LastSnap = 0;
	static float FrameTimeAvg = 0;
	int64 Now = time_get();
	char aBuffer[512];

	if(!g_Config.m_Debug)
		return;

	//m_pGraphics->BlendNormal();
	Graphics()->TextureSet(m_DebugFont);
	Graphics()->MapScreen(0,0,Graphics()->ScreenWidth(),Graphics()->ScreenHeight());

	if(time_get()-LastSnap > time_freq())
	{
		LastSnap = time_get();
		Prev = Current;
		net_stats(&Current);
	}

	/*
		eth = 14
		ip = 20
		udp = 8
		total = 42
	*/
	FrameTimeAvg = FrameTimeAvg*0.9f + m_FrameTime*0.1f;
	str_format(aBuffer, sizeof(aBuffer), "ticks: %8d %8d mem %dk %d  gfxmem: %dk  fps: %3d",
		m_CurGameTick, m_PredTick,
		mem_stats()->allocated/1024,
		mem_stats()->total_allocations,
		Graphics()->MemoryUsage()/1024,
		(int)(1.0f/FrameTimeAvg));
	Graphics()->QuadsText(2, 2, 16, 1,1,1,1, aBuffer);


	{
		int SendPackets = (Current.sent_packets-Prev.sent_packets);
		int SendBytes = (Current.sent_bytes-Prev.sent_bytes);
		int SendTotal = SendBytes + SendPackets*42;
		int RecvPackets = (Current.recv_packets-Prev.recv_packets);
		int RecvBytes = (Current.recv_bytes-Prev.recv_bytes);
		int RecvTotal = RecvBytes + RecvPackets*42;

		if(!SendPackets) SendPackets++;
		if(!RecvPackets) RecvPackets++;
		str_format(aBuffer, sizeof(aBuffer), "send: %3d %5d+%4d=%5d (%3d kbps) avg: %5d\nrecv: %3d %5d+%4d=%5d (%3d kbps) avg: %5d",
			SendPackets, SendBytes, SendPackets*42, SendTotal, (SendTotal*8)/1024, SendBytes/SendPackets,
			RecvPackets, RecvBytes, RecvPackets*42, RecvTotal, (RecvTotal*8)/1024, RecvBytes/RecvPackets);
		Graphics()->QuadsText(2, 14, 16, 1,1,1,1, aBuffer);
	}

	// render rates
	{
		int y = 0;
		int i;
		for(i = 0; i < 256; i++)
		{
			if(m_SnapshotDelta.GetDataRate(i))
			{
				str_format(aBuffer, sizeof(aBuffer), "%4d %20s: %8d %8d %8d", i, GameClient()->GetItemName(i), m_SnapshotDelta.GetDataRate(i)/8, m_SnapshotDelta.GetDataUpdates(i),
					(m_SnapshotDelta.GetDataRate(i)/m_SnapshotDelta.GetDataUpdates(i))/8);
				Graphics()->QuadsText(2, 100+y*12, 16, 1,1,1,1, aBuffer);
				y++;
			}
		}
	}

	str_format(aBuffer, sizeof(aBuffer), "pred: %d ms",
		(int)((m_PredictedTime.Get(Now)-m_GameTime.Get(Now))*1000/(float)time_freq()));
	Graphics()->QuadsText(2, 70, 16, 1,1,1,1, aBuffer);

	// render graphs
	if(g_Config.m_DbgGraphs)
	{
		//Graphics()->MapScreen(0,0,400.0f,300.0f);
		float w = Graphics()->ScreenWidth()/4.0f;
		float h = Graphics()->ScreenHeight()/6.0f;
		float sp = Graphics()->ScreenWidth()/100.0f;
		float x = Graphics()->ScreenWidth()-w-sp;

		m_FpsGraph.ScaleMax();
		m_FpsGraph.ScaleMin();
		m_FpsGraph.Render(Graphics(), m_DebugFont, x, sp*5, w, h, "FPS");
		m_InputtimeMarginGraph.Render(Graphics(), m_DebugFont, x, sp*5+h+sp, w, h, "Prediction Margin");
		m_GametimeMarginGraph.Render(Graphics(), m_DebugFont, x, sp*5+h+sp+h+sp, w, h, "Gametime Margin");
	}
}

void CClient::Quit()
{
	SetState(IClient::STATE_QUITING);
}

const char *CClient::ErrorString()
{
	return m_NetClient.ErrorString();
}

void CClient::Render()
{
	if(g_Config.m_GfxClear)
		Graphics()->Clear(1,1,0);

	GameClient()->OnRender();
	DebugRender();
}

const char *CClient::LoadMap(const char *pName, const char *pFilename, unsigned WantedCrc)
{
	static char aErrorMsg[128];

	SetState(IClient::STATE_LOADING);

	if(!m_pMap->Load(pFilename))
	{
		str_format(aErrorMsg, sizeof(aErrorMsg), "map '%s' not found", pFilename);
		return aErrorMsg;
	}

	// get the crc of the map
	if(m_pMap->Crc() != WantedCrc)
	{
		m_pMap->Unload();
		str_format(aErrorMsg, sizeof(aErrorMsg), "map differs from the server. %08x != %08x", m_pMap->Crc(), WantedCrc);
		return aErrorMsg;
	}

	// stop demo recording if we loaded a new map
	DemoRecorder_Stop();

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "loaded map '%s'", pFilename);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
	m_RecivedSnapshots = 0;

	str_copy(m_aCurrentMap, pName, sizeof(m_aCurrentMap));
	m_CurrentMapCrc = m_pMap->Crc();

	return 0x0;
}



const char *CClient::LoadMapSearch(const char *pMapName, int WantedCrc)
{
	const char *pError = 0;
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "loading map, map=%s wanted crc=%08x", pMapName, WantedCrc);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
	SetState(IClient::STATE_LOADING);

	// try the normal maps folder
	str_format(aBuf, sizeof(aBuf), "maps/%s.map", pMapName);
	pError = LoadMap(pMapName, aBuf, WantedCrc);
	if(!pError)
		return pError;

	// try the downloaded maps
	str_format(aBuf, sizeof(aBuf), "downloadedmaps/%s_%08x.map", pMapName, WantedCrc);
	pError = LoadMap(pMapName, aBuf, WantedCrc);
	return pError;
}

int CClient::PlayerScoreComp(const void *a, const void *b)
{
	CServerInfo::CPlayer *p0 = (CServerInfo::CPlayer *)a;
	CServerInfo::CPlayer *p1 = (CServerInfo::CPlayer *)b;
	if(p0->m_Score == p1->m_Score)
		return 0;
	if(p0->m_Score < p1->m_Score)
		return 1;
	return -1;
}

void CClient::ProcessPacket(CNetChunk *pPacket)
{
	if(pPacket->m_ClientID == -1)
	{
		// connectionlesss
		if(pPacket->m_DataSize == (int)(sizeof(VERSIONSRV_VERSION) + sizeof(VERSION_DATA)) &&
			mem_comp(pPacket->m_pData, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION)) == 0)
		{
			unsigned char *pVersionData = (unsigned char*)pPacket->m_pData + sizeof(VERSIONSRV_VERSION);
			int VersionMatch = !mem_comp(pVersionData, VERSION_DATA, sizeof(VERSION_DATA));

			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "version does %s (%d.%d.%d)",
				VersionMatch ? "match" : "NOT match",
				pVersionData[1], pVersionData[2], pVersionData[3]);
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/version", aBuf);

			// assume version is out of date when version-data doesn't match
			if (!VersionMatch)
			{
				str_format(m_aVersionStr, sizeof(m_aVersionStr), "%d.%d.%d", pVersionData[1], pVersionData[2], pVersionData[3]);
			}
		}

		if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_LIST) &&
			mem_comp(pPacket->m_pData, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST)) == 0)
		{
			int Size = pPacket->m_DataSize-sizeof(SERVERBROWSE_LIST);
			int Num = Size/sizeof(MASTERSRV_ADDR);
			MASTERSRV_ADDR *pAddrs = (MASTERSRV_ADDR *)((char*)pPacket->m_pData+sizeof(SERVERBROWSE_LIST));
			int i;

			for(i = 0; i < Num; i++)
			{
				NETADDR Addr;

				// convert address
				mem_zero(&Addr, sizeof(Addr));
				Addr.type = NETTYPE_IPV4;
				Addr.ip[0] = pAddrs[i].m_aIp[0];
				Addr.ip[1] = pAddrs[i].m_aIp[1];
				Addr.ip[2] = pAddrs[i].m_aIp[2];
				Addr.ip[3] = pAddrs[i].m_aIp[3];
				Addr.port = (pAddrs[i].m_aPort[1]<<8) | pAddrs[i].m_aPort[0];

				m_ServerBrowser.Set(Addr, IServerBrowser::SET_MASTER_ADD, -1, 0x0);
			}
		}

		{
			int PacketType = 0;
			if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_INFO) && mem_comp(pPacket->m_pData, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0)
				PacketType = 2;

			if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_OLD_INFO) && mem_comp(pPacket->m_pData, SERVERBROWSE_OLD_INFO, sizeof(SERVERBROWSE_OLD_INFO)) == 0)
				PacketType = 1;

			if(PacketType)
			{
				// we got ze info
				CUnpacker Up;
				CServerInfo Info = {0};
				int Token = -1;

				Up.Reset((unsigned char*)pPacket->m_pData+sizeof(SERVERBROWSE_INFO), pPacket->m_DataSize-sizeof(SERVERBROWSE_INFO));
				if(PacketType >= 2)
					Token = str_toint(Up.GetString());
				str_copy(Info.m_aVersion, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aVersion));
				str_copy(Info.m_aName, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aName));
				str_copy(Info.m_aMap, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aMap));
				str_copy(Info.m_aGameType, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aGameType));
				Info.m_Flags = str_toint(Up.GetString());
				Info.m_Progression = str_toint(Up.GetString());
				Info.m_NumPlayers = str_toint(Up.GetString());
				Info.m_MaxPlayers = str_toint(Up.GetString());

				// don't add invalid info to the server browser list
				if(Info.m_NumPlayers < 0 || Info.m_NumPlayers > MAX_CLIENTS || Info.m_MaxPlayers < 0 || Info.m_MaxPlayers > MAX_CLIENTS)
					return;

				str_format(Info.m_aAddress, sizeof(Info.m_aAddress), "%d.%d.%d.%d:%d",
					pPacket->m_Address.ip[0], pPacket->m_Address.ip[1], pPacket->m_Address.ip[2],
					pPacket->m_Address.ip[3], pPacket->m_Address.port);

				for(int i = 0; i < Info.m_NumPlayers; i++)
				{
					str_copy(Info.m_aPlayers[i].m_aName, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(Info.m_aPlayers[i].m_aName));
					Info.m_aPlayers[i].m_Score = str_toint(Up.GetString());
				}

				if(!Up.Error())
				{
					// sort players
					qsort(Info.m_aPlayers, Info.m_NumPlayers, sizeof(*Info.m_aPlayers), PlayerScoreComp);

					if(net_addr_comp(&m_ServerAddress, &pPacket->m_Address) == 0)
					{
						mem_copy(&m_CurrentServerInfo, &Info, sizeof(m_CurrentServerInfo));
						m_CurrentServerInfo.m_NetAddr = m_ServerAddress;
						m_CurrentServerInfoRequestTime = -1;
					}
					else
					{
						if(PacketType == 2)
							m_ServerBrowser.Set(pPacket->m_Address, IServerBrowser::SET_TOKEN, Token, &Info);
						else
							m_ServerBrowser.Set(pPacket->m_Address, IServerBrowser::SET_OLD_INTERNET, -1, &Info);
					}
				}
			}
		}
	}
	else
	{
		CUnpacker Unpacker;
		Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);

		// unpack msgid and system flag
		int Msg = Unpacker.GetInt();
		int Sys = Msg&1;
		Msg >>= 1;

		if(Unpacker.Error())
			return;

		if(Sys)
		{
			// system message
			if(Msg == NETMSG_MAP_CHANGE)
			{
				const char *pMap = Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
				int MapCrc = Unpacker.GetInt();
				const char *pError = 0;

				if(Unpacker.Error())
					return;

				for(int i = 0; pMap[i]; i++) // protect the player from nasty map names
				{
					if(pMap[i] == '/' || pMap[i] == '\\')
						pError = "strange character in map name";
				}

				if(pError)
					DisconnectWithReason(pError);
				else
				{
					pError = LoadMapSearch(pMap, MapCrc);

					if(!pError)
					{
						m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", "loading done");
						SendReady();
						GameClient()->OnConnected();
					}
					else
					{
						str_format(m_aMapdownloadFilename, sizeof(m_aMapdownloadFilename), "downloadedmaps/%s_%08x.map", pMap, MapCrc);

						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "starting to download map to '%s'", m_aMapdownloadFilename);
						m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", aBuf);

						m_MapdownloadChunk = 0;
						str_copy(m_aMapdownloadName, pMap, sizeof(m_aMapdownloadName));
						if(m_MapdownloadFile)
							io_close(m_MapdownloadFile);
						m_MapdownloadFile = Storage()->OpenFile(m_aMapdownloadFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
						m_MapdownloadCrc = MapCrc;
						m_MapdownloadTotalsize = -1;
						m_MapdownloadAmount = 0;

						CMsgPacker Msg(NETMSG_REQUEST_MAP_DATA);
						Msg.AddInt(m_MapdownloadChunk);
						SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);

						if(g_Config.m_Debug)
						{
							str_format(aBuf, sizeof(aBuf), "requested chunk %d", m_MapdownloadChunk);
							m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client/network", aBuf);
						}
					}
				}
			}
			else if(Msg == NETMSG_MAP_DATA)
			{
				int Last = Unpacker.GetInt();
				int TotalSize = Unpacker.GetInt();
				int Size = Unpacker.GetInt();
				const unsigned char *pData = Unpacker.GetRaw(Size);

				// check fior errors
				if(Unpacker.Error() || Size <= 0 || TotalSize <= 0 || !m_MapdownloadFile)
					return;

				io_write(m_MapdownloadFile, pData, Size);

				m_MapdownloadTotalsize = TotalSize;
				m_MapdownloadAmount += Size;

				if(Last)
				{
					const char *pError;
					m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", "download complete, loading map");

					if(m_MapdownloadFile)
						io_close(m_MapdownloadFile);
					m_MapdownloadFile = 0;
					m_MapdownloadAmount = 0;
					m_MapdownloadTotalsize = -1;

					// load map
					pError = LoadMap(m_aMapdownloadName, m_aMapdownloadFilename, m_MapdownloadCrc);
					if(!pError)
					{
						m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", "loading done");
						SendReady();
						GameClient()->OnConnected();
					}
					else
						DisconnectWithReason(pError);
				}
				else
				{
					// request new chunk
					m_MapdownloadChunk++;

					CMsgPacker Msg(NETMSG_REQUEST_MAP_DATA);
					Msg.AddInt(m_MapdownloadChunk);
					SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);

					if(g_Config.m_Debug)
					{
						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "requested chunk %d", m_MapdownloadChunk);
						m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client/network", aBuf);
					}
				}
			}
			else if(Msg == NETMSG_PING)
			{
				CMsgPacker Msg(NETMSG_PING_REPLY);
				SendMsgEx(&Msg, 0);
			}
			else if(Msg == NETMSG_RCON_AUTH_STATUS)
			{
				int Result = Unpacker.GetInt();
				if(Unpacker.Error() == 0)
					m_RconAuthed = Result;
			}
			else if(Msg == NETMSG_RCON_LINE)
			{
				const char *pLine = Unpacker.GetString();
				if(Unpacker.Error() == 0)
					GameClient()->OnRconLine(pLine);
			}
			else if(Msg == NETMSG_PING_REPLY)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "latency %.2f", (time_get() - m_PingStartTime)*1000 / (float)time_freq());
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client/network", aBuf);
			}
			else if(Msg == NETMSG_INPUTTIMING)
			{
				int InputPredTick = Unpacker.GetInt();
				int TimeLeft = Unpacker.GetInt();

				// adjust our prediction time
				int64 Target = 0;
				for(int k = 0; k < 200; k++)
				{
					if(m_aInputs[k].m_Tick == InputPredTick)
					{
						Target = m_aInputs[k].m_PredictedTime + (time_get() - m_aInputs[k].m_Time);
						Target = Target - (int64)(((TimeLeft-PREDICTION_MARGIN)/1000.0f)*time_freq());
						break;
					}
				}

				if(Target)
					m_PredictedTime.Update(&m_InputtimeMarginGraph, Target, TimeLeft, 1);
			}
			else if(Msg == NETMSG_SNAP || Msg == NETMSG_SNAPSINGLE || Msg == NETMSG_SNAPEMPTY)
			{
				int NumParts = 1;
				int Part = 0;
				int GameTick = Unpacker.GetInt();
				int DeltaTick = GameTick-Unpacker.GetInt();
				int PartSize = 0;
				int Crc = 0;
				int CompleteSize = 0;
				const char *pData = 0;

				// we are not allowed to process snapshot yet
				if(State() < IClient::STATE_LOADING)
					return;

				if(Msg == NETMSG_SNAP)
				{
					NumParts = Unpacker.GetInt();
					Part = Unpacker.GetInt();
				}

				if(Msg != NETMSG_SNAPEMPTY)
				{
					Crc = Unpacker.GetInt();
					PartSize = Unpacker.GetInt();
				}

				pData = (const char *)Unpacker.GetRaw(PartSize);

				if(Unpacker.Error())
					return;

				if(GameTick >= m_CurrentRecvTick)
				{
					if(GameTick != m_CurrentRecvTick)
					{
						m_SnapshotParts = 0;
						m_CurrentRecvTick = GameTick;
					}

					// TODO: clean this up abit
					mem_copy((char*)m_aSnapshotIncommingData + Part*MAX_SNAPSHOT_PACKSIZE, pData, PartSize);
					m_SnapshotParts |= 1<<Part;

					if(m_SnapshotParts == (unsigned)((1<<NumParts)-1))
					{
						static CSnapshot Emptysnap;
						CSnapshot *pDeltaShot = &Emptysnap;
						int PurgeTick;
						void *pDeltaData;
						int DeltaSize;
						unsigned char aTmpBuffer2[CSnapshot::MAX_SIZE];
						unsigned char aTmpBuffer3[CSnapshot::MAX_SIZE];
						CSnapshot *pTmpBuffer3 = (CSnapshot*)aTmpBuffer3;	// Fix compiler warning for strict-aliasing
						int SnapSize;

						CompleteSize = (NumParts-1) * MAX_SNAPSHOT_PACKSIZE + PartSize;

						// reset snapshoting
						m_SnapshotParts = 0;

						// find snapshot that we should use as delta
						Emptysnap.Clear();

						// find delta
						if(DeltaTick >= 0)
						{
							int DeltashotSize = m_SnapshotStorage.Get(DeltaTick, 0, &pDeltaShot, 0);

							if(DeltashotSize < 0)
							{
								// couldn't find the delta snapshots that the server used
								// to compress this snapshot. force the server to resync
								if(g_Config.m_Debug)
								{
									char aBuf[256];
									str_format(aBuf, sizeof(aBuf), "error, couldn't find the delta snapshot");
									m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
								}

								// ack snapshot
								// TODO: combine this with the input message
								m_AckGameTick = -1;
								return;
							}
						}

						// decompress snapshot
						pDeltaData = m_SnapshotDelta.EmptyDelta();
						DeltaSize = sizeof(int)*3;

						if(CompleteSize)
						{
							int IntSize = CVariableInt::Decompress(m_aSnapshotIncommingData, CompleteSize, aTmpBuffer2);

							if(IntSize < 0) // failure during decompression, bail
								return;

							pDeltaData = aTmpBuffer2;
							DeltaSize = IntSize;
						}

						// unpack delta
						PurgeTick = DeltaTick;
						SnapSize = m_SnapshotDelta.UnpackDelta(pDeltaShot, pTmpBuffer3, pDeltaData, DeltaSize);
						if(SnapSize < 0)
						{
							m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "delta unpack failed!");
							return;
						}

						if(Msg != NETMSG_SNAPEMPTY && pTmpBuffer3->Crc() != Crc)
						{
							if(g_Config.m_Debug)
							{
								char aBuf[256];
								str_format(aBuf, sizeof(aBuf), "snapshot crc error #%d - tick=%d wantedcrc=%d gotcrc=%d compressed_size=%d delta_tick=%d",
									m_SnapCrcErrors, GameTick, Crc, pTmpBuffer3->Crc(), CompleteSize, DeltaTick);
								m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
							}

							m_SnapCrcErrors++;
							if(m_SnapCrcErrors > 10)
							{
								// to many errors, send reset
								m_AckGameTick = -1;
								SendInput();
								m_SnapCrcErrors = 0;
							}
							return;
						}
						else
						{
							if(m_SnapCrcErrors)
								m_SnapCrcErrors--;
						}

						// purge old snapshots
						PurgeTick = DeltaTick;
						if(m_aSnapshots[SNAP_PREV] && m_aSnapshots[SNAP_PREV]->m_Tick < PurgeTick)
							PurgeTick = m_aSnapshots[SNAP_PREV]->m_Tick;
						if(m_aSnapshots[SNAP_CURRENT] && m_aSnapshots[SNAP_CURRENT]->m_Tick < PurgeTick)
							PurgeTick = m_aSnapshots[SNAP_PREV]->m_Tick;
						m_SnapshotStorage.PurgeUntil(PurgeTick);

						// add new
						m_SnapshotStorage.Add(GameTick, time_get(), SnapSize, pTmpBuffer3, 1);

						// add snapshot to demo
						if(m_DemoRecorder.IsRecording())
						{
							// write snapshot
							m_DemoRecorder.RecordSnapshot(GameTick, pTmpBuffer3, SnapSize);
						}

						// apply snapshot, cycle pointers
						m_RecivedSnapshots++;

						m_CurrentRecvTick = GameTick;

						// we got two snapshots until we see us self as connected
						if(m_RecivedSnapshots == 2)
						{
							// start at 200ms and work from there
							m_PredictedTime.Init(GameTick*time_freq()/50);
							m_PredictedTime.SetAdjustSpeed(1, 1000.0f);
							m_GameTime.Init((GameTick-1)*time_freq()/50);
							m_aSnapshots[SNAP_PREV] = m_SnapshotStorage.m_pFirst;
							m_aSnapshots[SNAP_CURRENT] = m_SnapshotStorage.m_pLast;
							m_LocalStartTime = time_get();
							SetState(IClient::STATE_ONLINE);
							DemoRecorder_HandleAutoStart();
						}

						// adjust game time
						{
							int64 Now = m_GameTime.Get(time_get());
							int64 TickStart = GameTick*time_freq()/50;
							int64 TimeLeft = (TickStart-Now)*1000 / time_freq();
							//st_update(&game_time, (game_tick-1)*time_freq()/50);
							m_GameTime.Update(&m_GametimeMarginGraph, (GameTick-1)*time_freq()/50, TimeLeft, 0);
						}

						// ack snapshot
						m_AckGameTick = GameTick;
					}
				}
			}
		}
		else
		{
			// game message
			if(m_DemoRecorder.IsRecording())
				m_DemoRecorder.RecordMessage(pPacket->m_pData, pPacket->m_DataSize);

			GameClient()->OnMessage(Msg, &Unpacker);
		}
	}
}

void CClient::PumpNetwork()
{
	m_NetClient.Update();

	if(State() != IClient::STATE_DEMOPLAYBACK)
	{
		// check for errors
		if(State() != IClient::STATE_OFFLINE && State() != IClient::STATE_QUITING && m_NetClient.State() == NETSTATE_OFFLINE)
		{
			SetState(IClient::STATE_OFFLINE);
			Disconnect();
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "offline error='%s'", m_NetClient.ErrorString());
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);
		}

		//
		if(State() == IClient::STATE_CONNECTING && m_NetClient.State() == NETSTATE_ONLINE)
		{
			// we switched to online
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", "connected, sending info");
			SetState(IClient::STATE_LOADING);
			SendInfo();
		}
	}

	// process packets
	CNetChunk Packet;
	while(m_NetClient.Recv(&Packet))
		ProcessPacket(&Packet);
}

void CClient::OnDemoPlayerSnapshot(void *pData, int Size)
{
	// update ticks, they could have changed
	const CDemoPlayer::CPlaybackInfo *pInfo = m_DemoPlayer.Info();
	CSnapshotStorage::CHolder *pTemp;
	m_CurGameTick = pInfo->m_Info.m_CurrentTick;
	m_PrevGameTick = pInfo->m_PreviousTick;

	// handle snapshots
	pTemp = m_aSnapshots[SNAP_PREV];
	m_aSnapshots[SNAP_PREV] = m_aSnapshots[SNAP_CURRENT];
	m_aSnapshots[SNAP_CURRENT] = pTemp;

	mem_copy(m_aSnapshots[SNAP_CURRENT]->m_pSnap, pData, Size);
	mem_copy(m_aSnapshots[SNAP_CURRENT]->m_pAltSnap, pData, Size);

	GameClient()->OnNewSnapshot();
}

void CClient::OnDemoPlayerMessage(void *pData, int Size)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pData, Size);

	// unpack msgid and system flag
	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(!Sys)
		GameClient()->OnMessage(Msg, &Unpacker);
}
/*
const IDemoPlayer::CInfo *client_demoplayer_getinfo()
{
	static DEMOPLAYBACK_INFO ret;
	const DEMOREC_PLAYBACKINFO *info = m_DemoPlayer.Info();
	ret.first_tick = info->first_tick;
	ret.last_tick = info->last_tick;
	ret.current_tick = info->current_tick;
	ret.paused = info->paused;
	ret.speed = info->speed;
	return &ret;
}*/

/*
void DemoPlayer()->SetPos(float percent)
{
	demorec_playback_set(percent);
}

void DemoPlayer()->SetSpeed(float speed)
{
	demorec_playback_setspeed(speed);
}

void DemoPlayer()->SetPause(int paused)
{
	if(paused)
		demorec_playback_pause();
	else
		demorec_playback_unpause();
}*/

void CClient::Update()
{
	if(State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_DemoPlayer.Update();
		if(m_DemoPlayer.IsPlaying())
		{
			// update timers
			const CDemoPlayer::CPlaybackInfo *pInfo = m_DemoPlayer.Info();
			m_CurGameTick = pInfo->m_Info.m_CurrentTick;
			m_PrevGameTick = pInfo->m_PreviousTick;
			m_GameIntraTick = pInfo->m_IntraTick;
			m_GameTickTime = pInfo->m_TickTime;
		}
		else
		{
			// disconnect on error
			Disconnect();
		}
	}
	else if(State() != IClient::STATE_OFFLINE && m_RecivedSnapshots >= 3)
	{
		// switch snapshot
		int Repredict = 0;
		int64 Freq = time_freq();
		int64 Now = m_GameTime.Get(time_get());
		int64 PredNow = m_PredictedTime.Get(time_get());

		while(1)
		{
			CSnapshotStorage::CHolder *pCur = m_aSnapshots[SNAP_CURRENT];
			int64 TickStart = (pCur->m_Tick)*time_freq()/50;

			if(TickStart < Now)
			{
				CSnapshotStorage::CHolder *pNext = m_aSnapshots[SNAP_CURRENT]->m_pNext;
				if(pNext)
				{
					m_aSnapshots[SNAP_PREV] = m_aSnapshots[SNAP_CURRENT];
					m_aSnapshots[SNAP_CURRENT] = pNext;

					// set ticks
					m_CurGameTick = m_aSnapshots[SNAP_CURRENT]->m_Tick;
					m_PrevGameTick = m_aSnapshots[SNAP_PREV]->m_Tick;

					if(m_aSnapshots[SNAP_CURRENT] && m_aSnapshots[SNAP_PREV])
					{
						GameClient()->OnNewSnapshot();
						Repredict = 1;
					}
				}
				else
					break;
			}
			else
				break;
		}

		if(m_aSnapshots[SNAP_CURRENT] && m_aSnapshots[SNAP_PREV])
		{
			int64 CurtickStart = (m_aSnapshots[SNAP_CURRENT]->m_Tick)*time_freq()/50;
			int64 PrevtickStart = (m_aSnapshots[SNAP_PREV]->m_Tick)*time_freq()/50;
			int PrevPredTick = (int)(PredNow*50/time_freq());
			int NewPredTick = PrevPredTick+1;
			static float LastPredintra = 0;

			m_GameIntraTick = (Now - PrevtickStart) / (float)(CurtickStart-PrevtickStart);
			m_GameTickTime = (Now - PrevtickStart) / (float)Freq; //(float)SERVER_TICK_SPEED);

			CurtickStart = NewPredTick*time_freq()/50;
			PrevtickStart = PrevPredTick*time_freq()/50;
			m_PredIntraTick = (PredNow - PrevtickStart) / (float)(CurtickStart-PrevtickStart);

			if(NewPredTick < m_aSnapshots[SNAP_PREV]->m_Tick-SERVER_TICK_SPEED || NewPredTick > m_aSnapshots[SNAP_PREV]->m_Tick+SERVER_TICK_SPEED)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", "prediction time reset!");
				m_PredictedTime.Init(m_aSnapshots[SNAP_CURRENT]->m_Tick*time_freq()/50);
			}

			if(NewPredTick > m_PredTick)
			{
				LastPredintra = m_PredIntraTick;
				m_PredTick = NewPredTick;
				Repredict = 1;

				// send input
				SendInput();
			}

			LastPredintra = m_PredIntraTick;
		}

		// only do sane predictions
		if(Repredict)
		{
			if(m_PredTick > m_CurGameTick && m_PredTick < m_CurGameTick+50)
				GameClient()->OnPredict();
		}

		// fetch server info if we don't have it
		if(State() >= IClient::STATE_LOADING &&
			m_CurrentServerInfoRequestTime >= 0 &&
			time_get() > m_CurrentServerInfoRequestTime)
		{
			m_ServerBrowser.Request(m_ServerAddress);
			m_CurrentServerInfoRequestTime = time_get()+time_freq()*2;
		}
	}

	// STRESS TEST: join the server again
	if(g_Config.m_DbgStress)
	{
		static int64 ActionTaken = 0;
		int64 Now = time_get();
		if(State() == IClient::STATE_OFFLINE)
		{
			if(Now > ActionTaken+time_freq()*2)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "stress", "reconnecting!");
				Connect(g_Config.m_DbgStressServer);
				ActionTaken = Now;
			}
		}
	}

	// pump the network
	PumpNetwork();

	// update the maser server registry
	MasterServer()->Update();

	// update the server browser
	m_ServerBrowser.Update();
}

void CClient::VersionUpdate()
{
	if(m_VersionInfo.m_State == 0)
	{
		m_Engine.HostLookup(&m_VersionInfo.m_VersionServeraddr, g_Config.m_ClVersionServer);
		m_VersionInfo.m_State++;
	}
	else if(m_VersionInfo.m_State == 1)
	{
		if(m_VersionInfo.m_VersionServeraddr.m_Job.Status() == CJob::STATE_DONE)
		{
			CNetChunk Packet;

			mem_zero(&Packet, sizeof(Packet));

			m_VersionInfo.m_VersionServeraddr.m_Addr.port = VERSIONSRV_PORT;

			Packet.m_ClientID = -1;
			Packet.m_Address = m_VersionInfo.m_VersionServeraddr.m_Addr;
			Packet.m_pData = VERSIONSRV_GETVERSION;
			Packet.m_DataSize = sizeof(VERSIONSRV_GETVERSION);
			Packet.m_Flags = NETSENDFLAG_CONNLESS;

			m_NetClient.Send(&Packet);
			m_VersionInfo.m_State++;
		}
	}
}

void CClient::InitEngine(const char *pAppname)
{
	m_Engine.Init(pAppname);
}

void CClient::RegisterInterfaces()
{
	Kernel()->RegisterInterface(static_cast<IDemoRecorder*>(&m_DemoRecorder));
	Kernel()->RegisterInterface(static_cast<IDemoPlayer*>(&m_DemoPlayer));
	Kernel()->RegisterInterface(static_cast<IServerBrowser*>(&m_ServerBrowser));
}

void CClient::InitInterfaces()
{
	// fetch interfaces
	m_pEditor = Kernel()->RequestInterface<IEditor>();
	m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();
	m_pSound = Kernel()->RequestInterface<IEngineSound>();
	m_pGameClient = Kernel()->RequestInterface<IGameClient>();
	m_pInput = Kernel()->RequestInterface<IEngineInput>();
	m_pMap = Kernel()->RequestInterface<IEngineMap>();
	m_pMasterServer = Kernel()->RequestInterface<IEngineMasterServer>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();

	//
	m_ServerBrowser.SetBaseInfo(&m_NetClient, m_pGameClient->NetVersion());
}

void CClient::Run()
{
	int64 ReportTime = time_get();
	int64 ReportInterval = time_freq()*1;

	m_LocalStartTime = time_get();
	m_SnapshotParts = 0;

	// init graphics
	if(m_pGraphics->Init() != 0)
		return;

	// init font rendering
	Kernel()->RequestInterface<IEngineTextRender>()->Init();

	// init the input
	Input()->Init();

	// start refreshing addresses while we load
	MasterServer()->RefreshAddresses();

	// init the editor
	m_pEditor->Init();

	// init sound, allowed to fail
	Sound()->Init();

	// load data
	if(!LoadData())
		return;

	DemoRecorder_Init();

	GameClient()->OnInit();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "version %s", GameClient()->NetVersion());
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);

	// open socket
	{
		NETADDR BindAddr;
		mem_zero(&BindAddr, sizeof(BindAddr));
		m_NetClient.Open(BindAddr, 0);
	}

	// connect to the server if wanted
	/*
	if(config.cl_connect[0] != 0)
		Connect(config.cl_connect);
	config.cl_connect[0] = 0;
	*/

	//
	m_FpsGraph.Init(0.0f, 200.0f);

	// never start with the editor
	g_Config.m_ClEditor = 0;

	Input()->MouseModeRelative();

	// process pending commands
	m_pConsole->StoreCommands(false, -1);

	while (1)
	{
		int64 FrameStartTime = time_get();
		m_Frames++;

		//
		VersionUpdate();

		// handle pending connects
		if(m_aCmdConnect[0])
		{
			str_copy(g_Config.m_UiServerAddress, m_aCmdConnect, sizeof(g_Config.m_UiServerAddress));
			Connect(m_aCmdConnect);
			m_aCmdConnect[0] = 0;
		}

		// update input
		if(Input()->Update())
			break;	// SDL_QUIT

		// update sound
		Sound()->Update();

		// release focus
		if(!m_pGraphics->WindowActive())
		{
			if(m_WindowMustRefocus == 0)
				Input()->MouseModeAbsolute();
			m_WindowMustRefocus = 1;
		}
		else if (g_Config.m_DbgFocus && Input()->KeyPressed(KEY_ESCAPE))
		{
			Input()->MouseModeAbsolute();
			m_WindowMustRefocus = 1;
		}

		// refocus
		if(m_WindowMustRefocus && m_pGraphics->WindowActive())
		{
			if(m_WindowMustRefocus < 3)
			{
				Input()->MouseModeAbsolute();
				m_WindowMustRefocus++;
			}

			if(m_WindowMustRefocus >= 3 || Input()->KeyPressed(KEY_MOUSE_1))
			{
				Input()->MouseModeRelative();
				m_WindowMustRefocus = 0;
			}
		}

		// panic quit button
		if(Input()->KeyPressed(KEY_LCTRL) && Input()->KeyPressed(KEY_LSHIFT) && Input()->KeyPressed('q'))
			break;

		if(Input()->KeyPressed(KEY_LCTRL) && Input()->KeyPressed(KEY_LSHIFT) && Input()->KeyDown('d'))
			g_Config.m_Debug ^= 1;

		if(Input()->KeyPressed(KEY_LCTRL) && Input()->KeyPressed(KEY_LSHIFT) && Input()->KeyDown('g'))
			g_Config.m_DbgGraphs ^= 1;

		if(Input()->KeyPressed(KEY_LCTRL) && Input()->KeyPressed(KEY_LSHIFT) && Input()->KeyDown('e'))
		{
			g_Config.m_ClEditor = g_Config.m_ClEditor^1;
			Input()->MouseModeRelative();
		}

		/*
		if(!gfx_window_open())
			break;
		*/

		// render
		if(g_Config.m_ClEditor)
		{
			Update();
			m_pEditor->UpdateAndRender();
			m_pGraphics->Swap();
		}
		else
		{
			Update();

			if(g_Config.m_DbgStress)
			{
				if((m_Frames%10) == 0)
				{
					Render();
					m_pGraphics->Swap();
				}
			}
			else
			{
				Render();
				m_pGraphics->Swap();
			}
		}

		AutoScreenshot_Cleanup();

		// check conditions
		if(State() == IClient::STATE_QUITING)
			break;

		// beNice
		if(g_Config.m_DbgStress)
			thread_sleep(5);
		else if(g_Config.m_ClCpuThrottle || !m_pGraphics->WindowActive())
			thread_sleep(1);

		if(g_Config.m_DbgHitch)
		{
			thread_sleep(g_Config.m_DbgHitch);
			g_Config.m_DbgHitch = 0;
		}

		if(ReportTime < time_get())
		{
			if(0 && g_Config.m_Debug)
			{
				dbg_msg("client/report", "fps=%.02f (%.02f %.02f) netstate=%d",
					m_Frames/(float)(ReportInterval/time_freq()),
					1.0f/m_FrameTimeHigh,
					1.0f/m_FrameTimeLow,
					m_NetClient.State());
			}
			m_FrameTimeLow = 1;
			m_FrameTimeHigh = 0;
			m_Frames = 0;
			ReportTime += ReportInterval;
		}

		// update frametime
		m_FrameTime = (time_get()-FrameStartTime)/(float)time_freq();
		if(m_FrameTime < m_FrameTimeLow)
			m_FrameTimeLow = m_FrameTime;
		if(m_FrameTime > m_FrameTimeHigh)
			m_FrameTimeHigh = m_FrameTime;

		m_LocalTime = (time_get()-m_LocalStartTime)/(float)time_freq();

		m_FpsGraph.Add(1.0f/m_FrameTime, 1,1,1);
	}

	GameClient()->OnShutdown();
	Disconnect();

	m_pGraphics->Shutdown();
	m_pSound->Shutdown();
}


void CClient::Con_Connect(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	str_copy(pSelf->m_aCmdConnect, pResult->GetString(0), sizeof(pSelf->m_aCmdConnect));
}

void CClient::Con_Disconnect(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->Disconnect();
}

void CClient::Con_Quit(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->Quit();
}

void CClient::Con_Minimize(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->Graphics()->Minimize();
}

void CClient::Con_Ping(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;

	CMsgPacker Msg(NETMSG_PING);
	pSelf->SendMsgEx(&Msg, 0);
	pSelf->m_PingStartTime = time_get();
}

void CClient::AutoScreenshot_Start()
{
	if(g_Config.m_ClAutoScreenshot)
	{
		Graphics()->TakeScreenshot("auto/autoscreen");
		m_AutoScreenshotRecycle = true;
	}
}

void CClient::AutoScreenshot_Cleanup()
{
	if(m_AutoScreenshotRecycle)
	{
		if(g_Config.m_ClAutoScreenshotMax)
		{
			// clean up auto taken screens
			CFileCollection AutoScreens;
			AutoScreens.Init(Storage(), "screenshots/auto", "autoscreen", ".png", g_Config.m_ClAutoScreenshotMax);
		}
		m_AutoScreenshotRecycle = false;
	}
}

void CClient::Con_Screenshot(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->Graphics()->TakeScreenshot(0);
}

void CClient::Con_Rcon(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->Rcon(pResult->GetString(0));
}

void CClient::Con_RconAuth(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->RconAuth("", pResult->GetString(0));
}

void CClient::Con_AddFavorite(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	NETADDR Addr;
	if(net_addr_from_str(&Addr, pResult->GetString(0)) == 0)
		pSelf->m_ServerBrowser.AddFavorite(Addr);
}

const char *CClient::DemoPlayer_Play(const char *pFilename, int StorageType)
{
	int Crc;
	const char *pError;
	Disconnect();
	m_NetClient.ResetErrorString();

	// try to start playback
	m_DemoPlayer.SetListner(this);

	if(m_DemoPlayer.Load(Storage(), m_pConsole, pFilename, StorageType))
		return "error loading demo";

	// load map
	Crc = (m_DemoPlayer.Info()->m_Header.m_aCrc[0]<<24)|
		(m_DemoPlayer.Info()->m_Header.m_aCrc[1]<<16)|
		(m_DemoPlayer.Info()->m_Header.m_aCrc[2]<<8)|
		(m_DemoPlayer.Info()->m_Header.m_aCrc[3]);
	pError = LoadMapSearch(m_DemoPlayer.Info()->m_Header.m_aMap, Crc);
	if(pError)
	{
		DisconnectWithReason(pError);
		return pError;
	}

	GameClient()->OnConnected();

	// setup buffers
	mem_zero(m_aDemorecSnapshotData, sizeof(m_aDemorecSnapshotData));

	m_aSnapshots[SNAP_CURRENT] = &m_aDemorecSnapshotHolders[SNAP_CURRENT];
	m_aSnapshots[SNAP_PREV] = &m_aDemorecSnapshotHolders[SNAP_PREV];

	m_aSnapshots[SNAP_CURRENT]->m_pSnap = (CSnapshot *)m_aDemorecSnapshotData[SNAP_CURRENT][0];
	m_aSnapshots[SNAP_CURRENT]->m_pAltSnap = (CSnapshot *)m_aDemorecSnapshotData[SNAP_CURRENT][1];
	m_aSnapshots[SNAP_CURRENT]->m_SnapSize = 0;
	m_aSnapshots[SNAP_CURRENT]->m_Tick = -1;

	m_aSnapshots[SNAP_PREV]->m_pSnap = (CSnapshot *)m_aDemorecSnapshotData[SNAP_PREV][0];
	m_aSnapshots[SNAP_PREV]->m_pAltSnap = (CSnapshot *)m_aDemorecSnapshotData[SNAP_PREV][1];
	m_aSnapshots[SNAP_PREV]->m_SnapSize = 0;
	m_aSnapshots[SNAP_PREV]->m_Tick = -1;

	// enter demo playback state
	SetState(IClient::STATE_DEMOPLAYBACK);

	m_DemoPlayer.Play();
	GameClient()->OnEnterGame();

	return 0;
}

void CClient::Con_Play(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->DemoPlayer_Play(pResult->GetString(0), IStorage::TYPE_ALL);
}

void CClient::DemoRecorder_Init()
{
	if(!Storage()->CreateFolder("demos/auto", IStorage::TYPE_SAVE))
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demorec/record", "unable to create auto record folder");
}

void CClient::DemoRecorder_Start(const char *pFilename, bool WithTimestamp)
{
	if(State() != IClient::STATE_ONLINE)
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demorec/record", "client is not online");
	else
	{
		char aFilename[128];
		if(WithTimestamp)
		{
			char aDate[20];
			str_timestamp(aDate, sizeof(aDate));
			str_format(aFilename, sizeof(aFilename), "demos/%s_%s.demo", pFilename, aDate);
		}
		else
			str_format(aFilename, sizeof(aFilename), "demos/%s.demo", pFilename);
		m_DemoRecorder.Start(Storage(), m_pConsole, aFilename, GameClient()->NetVersion(), m_aCurrentMap, m_CurrentMapCrc, "client");
	}
}

void CClient::DemoRecorder_HandleAutoStart()
{
	if(g_Config.m_ClAutoDemoRecord)
	{
		DemoRecorder_Start("auto/autorecord", true);
		if(g_Config.m_ClAutoDemoMax)
		{
			// clean up auto recorded demos
			CFileCollection AutoDemos;
			AutoDemos.Init(Storage(), "demos/auto", "autorecord", ".demo", g_Config.m_ClAutoDemoMax);
		}
	}
}

void CClient::DemoRecorder_Stop()
{
	m_DemoRecorder.Stop();
}

void CClient::Con_Record(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	if(pResult->NumArguments())
		pSelf->DemoRecorder_Start(pResult->GetString(0), false);
	else
		pSelf->DemoRecorder_Start("demo", true);
}

void CClient::Con_StopRecord(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->DemoRecorder_Stop();
}

void CClient::RegisterCommands()
{
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	// register server dummy commands for tab completion
	m_pConsole->Register("kick", "i?r", CFGFLAG_SERVER, 0, 0, "Kick player with specified id for any reason", 0);
	m_pConsole->Register("ban", "s?ir", CFGFLAG_SERVER, 0, 0, "Ban player with ip/id for x minutes for any reason", 0);
	m_pConsole->Register("unban", "s", CFGFLAG_SERVER, 0, 0, "Unban ip", 0);
	m_pConsole->Register("bans", "", CFGFLAG_SERVER, 0, 0, "Show banlist", 0);
	m_pConsole->Register("status", "", CFGFLAG_SERVER, 0, 0, "List players", 0);
	m_pConsole->Register("shutdown", "", CFGFLAG_SERVER, 0, 0, "Shut down", 0);
	m_pConsole->Register("record", "?s", CFGFLAG_SERVER, 0, 0, "Record to a file", 0);
	m_pConsole->Register("stoprecord", "", CFGFLAG_SERVER, 0, 0, "Stop recording", 0);
	m_pConsole->Register("reload", "", CFGFLAG_SERVER, 0, 0, "Reload the map", 0);
	m_pConsole->Register("login", "?s", CFGFLAG_SERVER, 0, 0, "Allows you access to rcon if no password is given, or changes your level if a password is given", -1);
	m_pConsole->Register("auth", "?s", CFGFLAG_SERVER, 0, 0, "Allows you access to rcon if no password is given, or changes your level if a password is given", -1);
	m_pConsole->Register("vote", "r", CFGFLAG_SERVER, 0, 0, "Forces the current vote to result in r (Yes/No)", 3);
	m_pConsole->Register("cmdlist", "", CFGFLAG_SERVER, 0, 0, "Shows the list of all commands", 0);

	#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help, level) m_pConsole->Register(name, params, flags, 0, 0, help, level);
	#include <game/ddracecommands.h>

	m_pConsole->Register("quit", "", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Quit, this, "Quit Teeworlds", 0);
	m_pConsole->Register("exit", "", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Quit, this, "Quit Teeworlds", 0);
	m_pConsole->Register("minimize", "", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Minimize, this, "Minimize Teeworlds", 0);
	m_pConsole->Register("connect", "s", CFGFLAG_CLIENT, Con_Connect, this, "Connect to the specified host/ip", 0);
	m_pConsole->Register("disconnect", "", CFGFLAG_CLIENT, Con_Disconnect, this, "Disconnect from the server", 0);
	m_pConsole->Register("ping", "", CFGFLAG_CLIENT, Con_Ping, this, "Ping the current server", 0);
	m_pConsole->Register("screenshot", "", CFGFLAG_CLIENT, Con_Screenshot, this, "Take a screenshot", 0);
	m_pConsole->Register("rcon", "r", CFGFLAG_CLIENT, Con_Rcon, this, "Send specified command to rcon", 0);
	m_pConsole->Register("rcon_auth", "s", CFGFLAG_CLIENT, Con_RconAuth, this, "Authenticate to rcon", 0);
	m_pConsole->Register("play", "r", CFGFLAG_CLIENT, Con_Play, this, "Play the file specified", 0);
	m_pConsole->Register("record", "?s", CFGFLAG_CLIENT, Con_Record, this, "Record to the file", 0);
	m_pConsole->Register("stoprecord", "", CFGFLAG_CLIENT, Con_StopRecord, this, "Stop recording", 0);
	
	m_pConsole->Register("add_favorite", "s", CFGFLAG_CLIENT, Con_AddFavorite, this, "Add a server as a favorite", 0);
}

static CClient m_Client;

/*
	Server Time
	Client Mirror Time
	Client Predicted Time

	Snapshot Latency
		Downstream latency

	Prediction Latency
		Upstream latency
*/

#if defined(CONF_PLATFORM_MACOSX)
extern "C" int SDL_main(int argc, const char **argv) // ignore_convention
#else
int main(int argc, const char **argv) // ignore_convention
#endif
{
#if defined(CONF_FAMILY_WINDOWS)
	for(int i = 1; i < argc; i++) // ignore_convention
	{
		if(str_comp("-s", argv[i]) == 0 || str_comp("--silent", argv[i]) == 0) // ignore_convention
		{
			FreeConsole();
			break;
		}
	}
#endif

	// init the engine
	dbg_msg("client", "starting...");
	m_Client.InitEngine("Teeworlds");

	IKernel *pKernel = IKernel::Create();
	pKernel->RegisterInterface(&m_Client);
	m_Client.RegisterInterfaces();

	// create the components
	IConsole *pConsole = CreateConsole(CFGFLAG_CLIENT);
	IStorage *pStorage = CreateStorage("Teeworlds", argc, argv); // ignore_convention
	IConfig *pConfig = CreateConfig();
	IEngineGraphics *pEngineGraphics = CreateEngineGraphics();
	IEngineSound *pEngineSound = CreateEngineSound();
	IEngineInput *pEngineInput = CreateEngineInput();
	IEngineTextRender *pEngineTextRender = CreateEngineTextRender();
	IEngineMap *pEngineMap = CreateEngineMap();
	IEngineMasterServer *pEngineMasterServer = CreateEngineMasterServer();

	{
		bool RegisterFail = false;

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IConsole*>(pConsole));
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IConfig*>(pConfig));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineGraphics*>(pEngineGraphics)); // register graphics as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IGraphics*>(pEngineGraphics));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineSound*>(pEngineSound)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<ISound*>(pEngineSound));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineInput*>(pEngineInput)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IInput*>(pEngineInput));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineTextRender*>(pEngineTextRender)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<ITextRender*>(pEngineTextRender));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMap*>(pEngineMap)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMap*>(pEngineMap));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMasterServer*>(pEngineMasterServer)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMasterServer*>(pEngineMasterServer));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(CreateEditor());
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(CreateGameClient());
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pStorage);

		if(RegisterFail)
			return -1;
	}

	pConfig->Init();
	pEngineMasterServer->Init(m_Client.Engine());
	pEngineMasterServer->Load();

	// register all console commands
	m_Client.RegisterCommands();

	pKernel->RequestInterface<IGameClient>()->OnConsoleInit();

	// init client's interfaces
	m_Client.InitInterfaces();

	// execute config file
	pConsole->ExecuteFile("settings.cfg");

	// execute autoexec file
	pConsole->ExecuteFile("autoexec.cfg");

	// parse the command line arguments
	if(argc > 1) // ignore_convention
		pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention

	// restore empty config strings to their defaults
	pConfig->RestoreStrings();

	m_Client.Engine()->InitLogfile();

	// run the client
	m_Client.Run();

	// write down the config and quit
	pConfig->Save();

	return 0;
}
