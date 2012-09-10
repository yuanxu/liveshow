#pragma once
#include "messagebase.h"

class MsgPing :
	public MessageBase
{
public:
	MsgPing(void);
	~MsgPing(void);

public:
	char ttl,hops;
	DWORD dwIP;
	PEERID dwSID,senderSID;
	WORD wPort;
	void fromBytes(const char*p,const int len);
	void toBytes();
};
