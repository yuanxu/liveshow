#include "StdAfx.h"
#include "MessageBase.h"

WLock* MessageBase::m_slock = new WLock();
WORD	MessageBase::m_sSeqID = 0;
