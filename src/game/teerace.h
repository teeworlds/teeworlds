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

#endif
