#pragma once
#include "messagebase.h"
#include <vector>

class MsgPushServant :
	public ReliableMessageBase
{
public:
	MsgPushServant(void);
	MsgPushServant(const char*p,const int len):ReliableMessageBase()
	{
		fromBytes(p,len);
	};
	~MsgPushServant(void);

	void fromBytes(const char* p,const int len);
	void toBytes(void);
public:
	std::vector<ServantInfo*> vtSvts;	
	
};

///////////////////////////////////////////////////////////

class MsgPushServantR :
	public ACKMessageBase
{
public:
	MsgPushServantR(WORD wSeq) :  ACKMessageBase()
	{
		wSeqID = wSeq;
		wCMD = CMD_PUSH_SERVANTS_R;
	}

	MsgPushServantR(const char*p,const int len)
		:ACKMessageBase(p,len)
	{
		wCMD = CMD_PUSH_SERVANTS_R;
	}

	~MsgPushServantR(void)
	{
	}

	void fromBytes(const char*p,const int len);
	void toBytes();
};
