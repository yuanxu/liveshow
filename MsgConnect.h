#pragma once
#include "Messagebase.h"
#include "msgExbm.h"

class MsgConnect :
	public ReliableMessageBase
{
public:
	MsgConnect(void);
	~MsgConnect(void);

	DWORD	dwSID;

public:
	void fromBytes(const char*p,const int len);
	void toBytes();
};

//--------------------------------------------------------------------------

class MsgConnectR :
	public MsgExBM
{
public:
	MsgConnectR(void);
	~MsgConnectR(void);
};

