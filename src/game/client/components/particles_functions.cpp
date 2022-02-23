#include <base/math.h>
#include <engine/graphics.h>
#include <engine/demo.h>

#include <generated/client_data.h>
#include <game/client/render.h>

#include "particles.h"
#include "water.h"

bool CParticle::CheckDeathConditions(CPointers* pPointers)
{
	// check particle death
	if (m_Life > m_LifeSpan)
	{
		return true;
	}

	return false;
}

bool CParticle::OnDeath(CPointers* pPointers)
{
	return true;
}

void CParticle::OnUpdate(CPointers* pPointers)
{
	return;
}

bool CBubbleParticle::OnDeath(CPointers* pPointers)
{
	m_BubbleStage--;
	float AdditionalLife = m_Water ? 0.1f : 0.025f;
	switch (m_BubbleStage)
	{
	case 3:
		m_Spr = SPRITE_PART_BUBBLE1;
		m_LifeSpan = m_Life + AdditionalLife;
		break;
	case 2:
		m_Spr = SPRITE_PART_BUBBLE2;
		m_LifeSpan = m_Life + AdditionalLife;
		break;
	case 1:
		m_Spr = SPRITE_PART_BUBBLE3;
		m_LifeSpan = m_Life + AdditionalLife;
		break;
	case 0:
		m_Spr = SPRITE_PART_BUBBLE4;
		m_LifeSpan = m_Life + AdditionalLife;
		break;
	default:
		return true; //kill the particle
		break;
	}
	
	return false;
}

void CBubbleParticle::OnUpdate(CPointers* pPointers)
{
	if (!pPointers->pCollision->TestBox(m_Pos, vec2(1.0f, 1.0f) * (2.0f / 3.0f), 8) && m_Water)
	{
		m_Water = false;
		m_LifeSpan = m_Life + 0.025f;
	}
	return;
}

bool CBubbleParticle::CheckDeathConditions(CPointers* pPointers)
{
	// check particle death
	if (m_Life > m_LifeSpan)
	{
		return true;
	}

	return false;
}

CDroplet::CDroplet()
{
	m_AlphaMultiplier = 5.0f;

	SetDefault();
}

bool CDroplet::CheckDeathConditions(CPointers* pPointers)
{
	if (m_Life > m_LifeSpan)
	{
		return true;
	}

	if(pPointers->pWater->IsUnderWater(m_Pos) && m_Life > 0.2f)
	{
		return true;
	}

	return false;
}

void CDroplet::OnUpdate(CPointers* pPointers)
{
	m_Rot = angle(vec2(m_Vel.y, m_Vel.x * -1));
	
	m_Color = pPointers->pWater->WaterColor() * (m_AlphaMultiplier / 5.0f);
}

void CParticle::Delete()
{
	delete this;
	return;
}

bool CDroplet::OnDeath(CPointers* pPointers)
{
	if (pPointers->pWater->IsUnderWater(m_Pos) && m_Life > 0.2f)
		return true;

	m_AlphaMultiplier -= pPointers->TimePassed;
	if (m_AlphaMultiplier < 0)
		return true;

	return false;
}