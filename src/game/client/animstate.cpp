/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/math.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include "animstate.h"

static void AnimSeqEval(CAnimSequence *pSeq, float Time, CAnimKeyframe *pFrame)
{
	if(pSeq->m_NumFrames == 0)
	{
		pFrame->m_Time = 0;
		pFrame->m_X = 0;
		pFrame->m_Y = 0;
		pFrame->m_Angle = 0;
	}
	else if(pSeq->m_NumFrames == 1)
	{
		*pFrame = pSeq->m_aFrames[0];
	}
	else
	{
		//time = max(0.0f, min(1.0f, time / duration)); // TODO: use clamp
		CAnimKeyframe *pFrame1 = 0;
		CAnimKeyframe *pFrame2 = 0;
		float Blend = 0.0f;

		// TODO: make this smarter.. binary search
		for (int i = 1; i < pSeq->m_NumFrames; i++)
		{
			if (pSeq->m_aFrames[i-1].m_Time <= Time && pSeq->m_aFrames[i].m_Time >= Time)
			{
				pFrame1 = &pSeq->m_aFrames[i-1];
				pFrame2 = &pSeq->m_aFrames[i];
				Blend = (Time - pFrame1->m_Time) / (pFrame2->m_Time - pFrame1->m_Time);
				break;
			}
		}

		if (pFrame1 && pFrame2)
		{
			pFrame->m_Time = Time;
			pFrame->m_X = mix(pFrame1->m_X, pFrame2->m_X, Blend);
			pFrame->m_Y = mix(pFrame1->m_Y, pFrame2->m_Y, Blend);
			pFrame->m_Angle = mix(pFrame1->m_Angle, pFrame2->m_Angle, Blend);
		}
	}
}

static void AnimAddKeyframe(CAnimKeyframe *pSeq, CAnimKeyframe *pAdded, float Amount)
{
	pSeq->m_X += pAdded->m_X*Amount;
	pSeq->m_Y += pAdded->m_Y*Amount;
	pSeq->m_Angle += pAdded->m_Angle*Amount;
}

static void AnimAdd(CAnimState *pState, CAnimState *pAdded, float Amount)
{
	AnimAddKeyframe(pState->GetBody(), pAdded->GetBody(), Amount);
	AnimAddKeyframe(pState->GetBackFoot(), pAdded->GetBackFoot(), Amount);
	AnimAddKeyframe(pState->GetFrontFoot(), pAdded->GetFrontFoot(), Amount);
	AnimAddKeyframe(pState->GetAttach(), pAdded->GetAttach(), Amount);
}


void CAnimState::Set(CAnimation *pAnim, float Time)
{
	AnimSeqEval(&pAnim->m_Body, Time, &m_Body);
	AnimSeqEval(&pAnim->m_BackFoot, Time, &m_BackFoot);
	AnimSeqEval(&pAnim->m_FrontFoot, Time, &m_FrontFoot);
	AnimSeqEval(&pAnim->m_Attach, Time, &m_Attach);
}

void CAnimState::Add(CAnimation *pAnim, float Time, float Amount)
{
	CAnimState Add;
	Add.Set(pAnim, Time);
	AnimAdd(this, &Add, Amount);
}

CAnimState *CAnimState::GetIdle()
{
	static CAnimState State;
	static bool Init = true;

	if(Init)
	{
		State.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);
		State.Add(&g_pData->m_aAnimations[ANIM_IDLE], 0, 1.0f);
		Init = false;
	}

	return &State;
}
