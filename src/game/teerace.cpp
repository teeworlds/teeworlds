/* Teerace class by Sushi and Redix */

#include <base/math.h>
#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/http.h>

#include "teerace.h"

const char *ITeerace::Host()
{
	return g_Config.m_WaWebappIp;
}

CBufferRequest *ITeerace::CreateApiRequest(int Method, const char *pURI)
{
	char aURI[256];
	str_format(aURI, sizeof(aURI), "%s%s%s", g_Config.m_WaApiPath, pURI[0] == '/' ? "" : "/", pURI);
	return new CBufferRequest(Method, aURI);
}

CFileRequest *ITeerace::CreateApiUpload(const char *pURI)
{
	char aURI[256];
	str_format(aURI, sizeof(aURI), "%s%s%s", g_Config.m_WaApiPath, pURI[0] == '/' ? "" : "/", pURI);
	return new CFileRequest(aURI);
}

int IRace::TimeFromSecondsStr(const char *pStr)
{
	int Time = str_toint(pStr) * 1000;
	while(*pStr == ' ') // skip leading spaces
		pStr++;
	while(*pStr >= '0' && *pStr <= '9')
		pStr++;
	if(*pStr == '.' || *pStr == ',')
	{
		pStr++;
		static const int s_aMult[3] = { 100, 10, 1 };
		for(int i = 0; pStr[i] >= '0' && pStr[i] <= '9' && i < 3; i++)
			Time += (pStr[i] - '0') * s_aMult[i];
	}
	return Time;
}

int IRace::TimeFromStr(const char *pStr)
{
	static const char *s_pMinutesStr = " minute(s) ";
	static const char *s_pSecondsStr = " second(s)";

	const char *pSeconds = str_find(pStr, s_pSecondsStr);
	if(!pSeconds)
		return 0;

	const char *pMinutes = str_find(pStr, s_pMinutesStr);
	if(pMinutes)
		return str_toint(pStr) * 60 * 1000 + TimeFromSecondsStr(pMinutes + str_length(s_pMinutesStr));
	else
		return TimeFromSecondsStr(pStr);
}

int IRace::TimeFromFinishMessage(const char *pStr, char *pNameBuf, int NameBufSize)
{
	static const char *s_pFinishedStr = " finished in: ";
	const char *pFinished = str_find(pStr, s_pFinishedStr);
	if(!pFinished)
		return 0;

	int FinishedPos = pFinished - pStr;
	if(FinishedPos == 0 || FinishedPos >= NameBufSize)
		return 0;

	str_copy(pNameBuf, pStr, FinishedPos + 1);

	return TimeFromStr(pFinished + str_length(s_pFinishedStr));
}

void IRace::FormatTimeLong(char *pBuf, int Size, int Time, bool ForceMinutes)
{
	if(!ForceMinutes && Time < 60 * 1000)
		str_format(pBuf, Size, "%d.%03d second(s)", Time / 1000, Time % 1000);
	else
		str_format(pBuf, Size, "%d minute(s) %d.%03d second(s)", Time / (60 * 1000), (Time / 1000) % 60, Time % 1000);
}

void IRace::FormatTimeShort(char *pBuf, int Size, int Time, bool ForceMinutes)
{
	if(!ForceMinutes && Time < 60 * 1000)
		str_format(pBuf, Size, "%d.%03d", Time / 1000, Time % 1000);
	else
		str_format(pBuf, Size, "%02d:%02d.%03d", Time / (60 * 1000), (Time / 1000) % 60, Time % 1000);
}

void IRace::FormatTimeDiff(char *pBuf, int Size, int Time, bool Milli)
{
	int PosDiff = absolute(Time);
	char Sign = Time < 0 ? '-' : '+';
	if(Milli)
		str_format(pBuf, Size, "%c%d.%03d", Sign, PosDiff / 1000, PosDiff % 1000);
	else
		str_format(pBuf, Size, "%c%d.%02d", Sign, PosDiff / 1000, (PosDiff % 1000) / 10);
}
