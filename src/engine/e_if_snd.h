/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_SND_H
#define ENGINE_IF_SND_H

/*
	Section: Sound
*/

/*
	Function: snd_set_channel
		Sets the parameters for a sound channel.
	
	Arguments:
		cid - Channel ID
		vol - Volume for the channel. 0.0 to 1.0.
		pan - Panning for the channel. -1.0 is all left. 0.0 is equal distribution. 1.0 is all right.
*/
void snd_set_channel(int cid, float vol, float pan);

/*
	Function: snd_load_wv
		Loads a wavpack compressed sound.
	
	Arguments:
		filename - Filename of the file to load
	
	Returns:
		The id of the loaded sound. -1 on failure.
*/
int snd_load_wv(const char *filename);

/*
	Function: snd_play_at
		Plays a sound at a specified postition.
	
	Arguments:
		cid - Channel id of the channel to use.
		sid - Sound id of the sound to play.
		flags - TODO
		x - TODO
		y - TODO
	
	Returns:
		An id to the voice. -1 on failure.

	See Also:
		<snd_play, snd_stop>
*/
int snd_play_at(int cid, int sid, int flags, float x, float y);

/*
	Function: snd_play
		Plays a sound.
	
	Arguments:
	Arguments:
		cid - Channel id of the channel to use.
		sid - Sound id of the sound to play.
		flags - TODO
	
	Returns:
		An id to the voice. -1 on failure.

	See Also:
		<snd_play_at, snd_stop>
*/
int snd_play(int cid, int sid, int flags);

/*
	Function: snd_stop
		Stops a currenly playing sound.
	
	Arguments:
		id - The ID of the voice to stop.
	
	See Also:
		<snd_play, snd_play_at>
*/
void snd_stop(int id);

/*
	Function: snd_set_listener_pos
		Sets the listener posititon.
	
	Arguments:
		x - TODO
		y - TODO
*/
void snd_set_listener_pos(float x, float y);

#endif
