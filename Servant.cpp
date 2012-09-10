#include "StdAfx.h"
#include "Servant.h"


Servant::~Servant(void)
{
	//TODO:Ïú»ÙSocket
	delete m_BM;
	//delete m_Lock;
//	WSACloseEvent(m_Event);
}



void Servant::SyncID(DWORD id)
{
	if (NULL == lpSktObj)
	{
		return;
	}
	lpSktObj->SyncID(id);
}


//-------------------------------
ServantList::ServantList():m_Root(NULL),m_Tail(NULL)
{
	Count =0;
	m_Lock = new WLock();
}

ServantList::~ServantList()
{
	delete m_Lock;
}