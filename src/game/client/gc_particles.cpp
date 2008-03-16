#include <engine/e_client_interface.h>
#include "gc_client.h"
#include "../generated/gc_data.h"

// NOTE: the way the particle system works isn't very cache friendly

enum
{
	MAX_PARTICLES=1024*8,
};

static particle particles[MAX_PARTICLES];
static int first_free = -1;
static int first_part[NUM_PARTGROUPS] = {-1};

void particle_reset()
{
	// reset particles
	for(int i = 0; i < MAX_PARTICLES; i++)
	{
		particles[i].prev_part = i-1;
		particles[i].next_part = i+1;
	}
	
	particles[0].prev_part = 0;
	particles[MAX_PARTICLES-1].next_part = -1;
	first_free = 0;

	for(int i = 0; i < NUM_PARTGROUPS; i++)
		first_part[i] = -1;
}


void particle_add(int group, particle *part)
{
	if (first_free == -1)
		return;
		
	// remove from the free list
	int id = first_free;
	first_free = particles[id].next_part;
	particles[first_free].prev_part = -1;
	
	// copy data
	particles[id] = *part;
	
	// insert to the group list
	particles[id].prev_part = -1;
	particles[id].next_part = first_part[group];
	if(first_part[group] != -1)
		particles[first_part[group]].prev_part = id;
	first_part[group] = id;
	
	// set some parameters
	particles[id].life = 0;
}

void particle_update(float time_passed)
{
	static float friction_fraction = 0;
	friction_fraction += time_passed;

	if(friction_fraction > 2.0f) // safty messure
		friction_fraction = 0;
	
	int friction_count = 0;
	while(friction_fraction > 0.05f)
	{
		friction_count++;
		friction_fraction -= 0.05f;
	}
	
	for(int g = 0; g < NUM_PARTGROUPS; g++)
	{
		int i = first_part[g];
		while(i != -1)
		{
			int next = particles[i].next_part;
			particles[i].vel += flow_get(particles[i].pos)*time_passed * particles[i].flow_affected;
			particles[i].vel.y += particles[i].gravity*time_passed;
			
			for(int f = 0; f < friction_count; f++) // apply friction
				particles[i].vel *= particles[i].friction;
			
			// move the point
			vec2 vel = particles[i].vel*time_passed;
			move_point(&particles[i].pos, &vel, 0.1f+0.9f*frandom(), NULL);
			particles[i].vel = vel* (1.0f/time_passed);
			
			particles[i].life += time_passed;
			particles[i].rot += time_passed * particles[i].rotspeed;

			// check particle death
			if(particles[i].life > particles[i].life_span)
			{
				// remove it from the group list
				if(particles[i].prev_part != -1)
					particles[particles[i].prev_part].next_part = particles[i].next_part;
				else
					first_part[g] = particles[i].next_part;
					
				if(particles[i].next_part != -1)
					particles[particles[i].next_part].prev_part = particles[i].prev_part;
					
				// insert to the free list
				if(first_free != -1)
					particles[first_free].prev_part = i;
				particles[i].prev_part = -1;
				particles[i].next_part = first_free;
				first_free = i;
			}
			
			i = next;
		}
	}
}

void particle_render(int group)
{
	gfx_blend_normal();
	//gfx_blend_additive();
	gfx_texture_set(data->images[IMAGE_PARTICLES].id);
	gfx_quads_begin();

	int i = first_part[group];
	while(i != -1)
	{
		select_sprite(particles[i].spr);
		float a = particles[i].life / particles[i].life_span;
		vec2 p = particles[i].pos;
		float size = mix(particles[i].start_size, particles[i].end_size, a);

		gfx_quads_setrotation(particles[i].rot);

		gfx_setcolor(
			particles[i].color.r,
			particles[i].color.g,
			particles[i].color.b,
			particles[i].color.a); // pow(a, 0.75f) * 

		gfx_quads_draw(p.x, p.y, size, size);
		
		i = particles[i].next_part;
	}
	gfx_quads_end();
	gfx_blend_normal();
}
