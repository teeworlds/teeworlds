/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "entity.h"
#include "entitylist.h"

CEntityList::CEntityList()
{
	m_pFirst = 0;
	m_Size = 0;
	m_pIterator = 0;
}

CEntityList::~CEntityList()
{
	DeleteAll();
}

void CEntityList::SetIterator(CIterator *pIterator)
{
	dbg_assert(m_pIterator == 0, "An iterator is already set for this entity list");
	m_pIterator = pIterator;
}

void CEntityList::UnsetIterator(CIterator *pIterator)
{
	dbg_assert(m_pIterator == pIterator, "This iterator was not set for this entity list");
	m_pIterator = 0;
}

bool CEntityList::Contains(CEntity *pEnt) const
{
	for(CEntity *e = m_pFirst; e; e = e->GetNextEntity())
	{
		if(e == pEnt)
			return true;
	}
	return false;
}

void CEntityList::Insert(CEntity *pEnt)
{
#ifdef CONF_DEBUG
	dbg_assert(!Contains(pEnt), "This entity is already in the list");
#endif

	if(m_pFirst)
		m_pFirst->SetPrevEntity(pEnt);
	pEnt->SetNextEntity(m_pFirst);
	pEnt->SetPrevEntity(0);
	m_pFirst = pEnt;
	m_Size++;
}

void CEntityList::Remove(CEntity *pEnt)
{
#ifdef CONF_DEBUG
	dbg_assert(Contains(pEnt), "This entity is not in the list");
#endif

	// remove
	if(pEnt->GetPrevEntity())
		pEnt->GetPrevEntity()->SetNextEntity(pEnt->GetNextEntity());
	else
		m_pFirst = pEnt->GetNextEntity();
	if(pEnt->GetNextEntity())
		pEnt->GetNextEntity()->SetPrevEntity(pEnt->GetPrevEntity());

	// keep list traversing valid
	if(m_pIterator)
		m_pIterator->OnRemove(pEnt);

	pEnt->SetNextEntity(0);
	pEnt->SetPrevEntity(0);

	m_Size--;
}

void CEntityList::DeleteAll()
{
	CEntity *pEnt = m_pFirst;
	while(pEnt)
	{
		CEntity *pNext = pEnt->GetNextEntity();

		pEnt->MarkForDestroy();
		delete pEnt;

		pEnt = pNext;
	}
	m_pFirst = 0;
	m_Size = 0;
}


CEntityList::CIterator::CIterator(CEntityList &List)
{
	m_pList = &List;
	m_pCurrent = List.GetFirst();
	SetNext();
	List.SetIterator(this);
}

CEntityList::CIterator::~CIterator()
{
	m_pList->UnsetIterator(this);
}

void CEntityList::CIterator::SetNext()
{
	if(m_pCurrent)
		m_pNext = m_pCurrent->GetNextEntity();
	else
		m_pNext = 0;
}

void CEntityList::CIterator::OnRemove(CEntity *pEnt)
{
	if(m_pNext == pEnt)
		m_pNext = pEnt->GetNextEntity();
}
