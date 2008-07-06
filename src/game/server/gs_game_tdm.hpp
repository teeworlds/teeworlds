/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
// game object
class GAMECONTROLLER_TDM : public GAMECONTROLLER
{
public:
	GAMECONTROLLER_TDM();
	
	int on_character_death(class CHARACTER *victim, class PLAYER *killer, int weapon);
	virtual void tick();
};
