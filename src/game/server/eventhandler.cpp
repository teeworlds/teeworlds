#include "eventhandler.hpp"
#include "gamecontext.hpp"

//////////////////////////////////////////////////
// Event handler
//////////////////////////////////////////////////
EVENTHANDLER::EVENTHANDLER()
{
	clear();
}

void *EVENTHANDLER::create(int type, int size, int mask)
{
	if(num_events == MAX_EVENTS)
		return 0;
	if(current_offset+size >= MAX_DATASIZE)
		return 0;

	void *p = &data[current_offset];
	offsets[num_events] = current_offset;
	types[num_events] = type;
	sizes[num_events] = size;
	client_masks[num_events] = mask;
	current_offset += size;
	num_events++;
	return p;
}

void EVENTHANDLER::clear()
{
	num_events = 0;
	current_offset = 0;
}

void EVENTHANDLER::snap(int snapping_client)
{
	for(int i = 0; i < num_events; i++)
	{
		if(cmask_is_set(client_masks[i], snapping_client))
		{
			NETEVENT_COMMON *ev = (NETEVENT_COMMON *)&data[offsets[i]];
			if(distance(game.players[snapping_client]->view_pos, vec2(ev->x, ev->y)) < 1500.0f)
			{
				void *d = snap_new_item(types[i], i, sizes[i]);
				mem_copy(d, &data[offsets[i]], sizes[i]);
			}
		}
	}
}
