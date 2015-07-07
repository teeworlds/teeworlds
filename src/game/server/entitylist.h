/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITYLIST_H
#define GAME_SERVER_ENTITYLIST_H

class CEntity;

class CEntityList
{
public:
	/* Types */
	class CIterator // safe iterator in case an entity is removed while iterating through the list
	{
	private:
		/* Identity */
		CEntityList *m_pList;

		/* State */
		CEntity *m_pCurrent;
		CEntity *m_pNext;

		/* Functions */
		void SetNext();

	public:
		/* Constructor */
		CIterator(CEntityList &List);

		/* Destructor */
		~CIterator();

		/* Functions */
		bool Exists() const { return m_pCurrent != 0; }
		CEntity *Get() { return m_pCurrent; }
		void Next() { m_pCurrent = m_pNext; SetNext(); }
		void OnRemove(CEntity *pEnt);
	};

private:
	/* State */
	CEntity *m_pFirst;
	int m_Size;
	CIterator *m_pIterator;

public:
	/* Constructor */
	CEntityList();

	/* Destructor */
	~CEntityList();

	/* Getters */
	CEntity *GetFirst()	{ return m_pFirst; }
	int GetSize() const	{ return m_Size; }

	/* Functions (iterator) */
	void SetIterator(CIterator *pIterator);
	void UnsetIterator(CIterator *pIterator);

	/* Functions (user) */
	bool Contains(CEntity *pEnt) const;
	void Insert(CEntity *pEnt);
	void Remove(CEntity *pEnt);
	void DeleteAll();
};

#endif
