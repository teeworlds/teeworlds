/* Teerace Class by Sushi and Redix */
#ifndef GAME_TEERACE_H
#define GAME_TEERACE_H

// helper class
class ITeerace
{
public:
	static const char *Host();
	static class CRequest *CreateApiRequest(int Method, const char *pURI);
};

#endif
