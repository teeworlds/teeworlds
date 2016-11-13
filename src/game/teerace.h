/* Teerace Class by Sushi and Redix */
#ifndef GAME_TEERACE_H
#define GAME_TEERACE_H

// helper class
class ITeerace
{
public:
	static const char *Host();
	static class CBufferRequest *CreateApiRequest(int Method, const char *pURI);
	static class CFileRequest *CreateApiUpload(const char *pURI);
};

class IRace
{
public:
	static int TimeFromStr(const char *pStr);
	static int TimeFromFinishMessage(const char *pStr, char *pNameBuf, int NameBufSize);

	static void FormatTimeLong(char *pBuf, int Size, int Time, bool ForceMinutes = false);
	static void FormatTimeShort(char *pBuf, int Size, int Time, bool ForceMinutes = false);
	static void FormatTimeSeconds(char *pBuf, int Size, int Time);
	static void FormatTimeDiff(char *pBuf, int Size, int Time);
};

#endif
