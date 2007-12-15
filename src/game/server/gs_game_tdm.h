/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
// game object
class gameobject_tdm : public gameobject
{
public:
	gameobject_tdm();
	
	int on_player_death(class player *victim, class player *killer, int weapon);
	virtual void tick();
};
