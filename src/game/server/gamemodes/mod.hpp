/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include <game/server/gamecontroller.hpp>

// you can subclass GAMECONTROLLER_CTF, GAMECONTROLLER_TDM etc if you want
// todo a modification with their base as well.
class GAMECONTROLLER_MOD : public GAMECONTROLLER
{
public:
	GAMECONTROLLER_MOD();
	virtual void tick();
	// add more virtual functions here if you wish
};
