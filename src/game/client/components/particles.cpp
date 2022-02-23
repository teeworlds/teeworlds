/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <engine/graphics.h>
#include <engine/demo.h>

#include <generated/client_data.h>
#include <game/client/render.h>

#include "particles.h"
#include "water.h"

CParticles::CParticles()
{
	OnReset();
	m_RenderTrail.m_pParts = this;
	m_RenderExplosions.m_pParts = this;
	m_RenderGeneral.m_pParts = this;
}

void CParticles::OnReset()
{
	// reset particles
	for(int i = 0; i < MAX_PARTICLES; i++)
	{
		m_aParticles[i] = new CParticle();
		m_aParticles[i]->m_PrevPart = i-1;
		m_aParticles[i]->m_NextPart = i+1;
	}

	m_aParticles[0]->m_PrevPart = 0;
	m_aParticles[MAX_PARTICLES-1]->m_NextPart = -1;
	m_FirstFree = 0;

	for(int i = 0; i < NUM_GROUPS; i++)
		m_aFirstPart[i] = -1;
}

void CParticles::Add(int Group, CParticle *pPart)
{
	if(m_pClient->IsWorldPaused() || m_pClient->IsDemoPlaybackPaused())
		return;
	if (m_FirstFree == -1)
		return;

	// remove from the free list
	int Id = m_FirstFree;
	m_FirstFree = m_aParticles[Id]->m_NextPart;
	if(m_FirstFree != -1)
		m_aParticles[m_FirstFree]->m_PrevPart = -1;

	// copy data
	CParticle* ToDelete = m_aParticles[Id];
	m_aParticles[Id] = pPart;

	// insert to the group list
	m_aParticles[Id]->m_PrevPart = -1;
	m_aParticles[Id]->m_NextPart = m_aFirstPart[Group];
	if(m_aFirstPart[Group] != -1)
		m_aParticles[m_aFirstPart[Group]]->m_PrevPart = Id;
	m_aFirstPart[Group] = Id;

	// set some parameters
	m_aParticles[Id]->m_Life = 0;

	ToDelete->Delete();
}

void CParticles::Update(float TimePassed)
{
	if(TimePassed <= 0.0f)
		return;

	m_Pointers.pWater = m_pClient->m_pWater;
	m_Pointers.pCollision = Collision();
	m_Pointers.TimePassed = TimePassed;
	static float s_FrictionFraction = 0.0f;
	s_FrictionFraction += TimePassed;

	if(s_FrictionFraction > 2.0f) // safety messure
		s_FrictionFraction = 0.0f;

	int FrictionCount = 0;
	while(s_FrictionFraction > 0.05f)
	{
		FrictionCount++;
		s_FrictionFraction -= 0.05f;
	}

	for(int g = 0; g < NUM_GROUPS; g++)
	{
		int i = m_aFirstPart[g];
		while(i != -1)
		{
			int Next = m_aParticles[i]->m_NextPart;
			//m_aParticles[i]->vel += flow_get(m_aParticles[i]->pos)*time_passed * m_aParticles[i]->flow_affected;
			m_aParticles[i]->m_Vel.y += m_aParticles[i]->m_Gravity*TimePassed;

			for(int f = 0; f < FrictionCount; f++) // apply friction
				m_aParticles[i]->m_Vel *= m_aParticles[i]->m_Friction;

			// move the point
			vec2 Vel = m_aParticles[i]->m_Vel*TimePassed;
			int AmountofBounces = 0;
			if (m_aParticles[i]->m_Elasticity>=0)
				Collision()->MovePoint(&m_aParticles[i]->m_Pos, &Vel, m_aParticles[i]->m_Elasticity, &AmountofBounces);
			else
				Collision()->MovePoint(&m_aParticles[i]->m_Pos, &Vel, 0.1f + 0.9f * random_float(), &AmountofBounces);
			//if (m_aParticles[i]->m_Flags & PFLAG_DESTROY_ON_IMPACT && AmountofBounces)
			//{
			//	m_aParticles[i]->m_Life = m_aParticles[i]->m_LifeSpan + 1;
			//}
			m_aParticles[i]->m_Vel = Vel* (1.0f/TimePassed);

			m_aParticles[i]->m_Life += TimePassed;
			m_aParticles[i]->m_Rot += TimePassed * m_aParticles[i]->m_Rotspeed;
			
			if (m_aParticles[i]->CheckDeathConditions(&m_Pointers))
			{
				if (m_aParticles[i]->OnDeath(&m_Pointers)) //If the function returns 1, the particle is removed.
				{
					RemoveParticle(g, i);
				}
			}
			//else if (m_aParticles[i]->m_Flags & PFLAG_DESTROY_IN_WATER && Collision()->TestBox(m_aParticles[i]->m_Pos, vec2(12.0f, 12.0f), 8))
			//{
			//	RemoveParticle(g, i);
			//}
			m_aParticles[i]->OnUpdate(&m_Pointers);
			i = Next;
		}
	}
}

void CParticles::RemoveParticle(int Group, int Entry)
{
	//remove from the group list
	if (m_aParticles[Entry]->m_PrevPart != -1)
		m_aParticles[m_aParticles[Entry]->m_PrevPart]->m_NextPart = m_aParticles[Entry]->m_NextPart;
	else
		m_aFirstPart[Group] = m_aParticles[Entry]->m_NextPart;

	if (m_aParticles[Entry]->m_NextPart != -1)
		m_aParticles[m_aParticles[Entry]->m_NextPart]->m_PrevPart = m_aParticles[Entry]->m_PrevPart;

	// insert to the free list
	if (m_FirstFree != -1)
		m_aParticles[m_FirstFree]->m_PrevPart = Entry;
	m_aParticles[Entry]->m_PrevPart = -1;
	m_aParticles[Entry]->m_NextPart = m_FirstFree;
	m_FirstFree = Entry;
}

void CParticles::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	int64 Now = time_get();
	static int64 s_LastTime = Now;
	Update((float)((Now-s_LastTime)/(double)time_freq()) * m_pClient->GetAnimationPlaybackSpeed());
	s_LastTime = Now;
}

void CParticles::RenderGroup(int Group)
{
	Graphics()->BlendNormal();
	//gfx_blend_additive();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
	Graphics()->QuadsBegin();
	int i = m_aFirstPart[Group];
	while(i != -1)
	{
		RenderTools()->SelectSprite(m_aParticles[i]->m_Spr);
		float a = m_aParticles[i]->m_Life / m_aParticles[i]->m_LifeSpan;
		vec2 p = m_aParticles[i]->m_Pos;
		float Size = mix(m_aParticles[i]->m_StartSize, m_aParticles[i]->m_EndSize, a);
		Graphics()->QuadsSetRotation(m_aParticles[i]->m_Rot);

		Graphics()->SetColor(
			m_aParticles[i]->m_Color.r,
			m_aParticles[i]->m_Color.g,
			m_aParticles[i]->m_Color.b,
			m_aParticles[i]->m_Color.a
		); // pow(a, 0.75f) *
		//Graphics()->SetColor(0, 157.0f / 255.0f * 0.5, 1*0.5, 1 * 0.5);
		
		IGraphics::CQuadItem QuadItem(p.x, p.y, Size, Size);
		Graphics()->QuadsDraw(&QuadItem, 1);

		i = m_aParticles[i]->m_NextPart;
	}
	Graphics()->QuadsEnd();
	Graphics()->BlendNormal();
}