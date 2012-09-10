#pragma once
#include "messagebase.h"

class MsgPushData :
	public ReliableMessageBase
{
public:
	MsgPushData(void);
	MsgPushData(const char *p,const int len)
		:ReliableMessageBase()
	{
		wCMD = CMD_PUSH_DATA;
		lpData = NULL;
		cSeq = 0;
		dataLen = -1;
		fromBytes(p,len);
	};
	~MsgPushData(void);

public:
	SEGMENTID dwSegID;
	char cSeq;
	char* lpData;
	int dataLen;

public:
	void fromBytes(const char*p,const int len);
	void toBytes();
};

///////////////////////////////////////////////////////////

class MsgPushDataR :
	public ACKMessageBase
{
public:
	MsgPushDataR(WORD wSeq) :  ACKMessageBase()
	{
		wSeqID = wSeq;
		wCMD = CMD_PUSH_DATA_R;
	}

	MsgPushDataR(const char*p,const int len)
		:ACKMessageBase(p,len)
	{
		wCMD = CMD_PUSH_DATA_R;
	}

	~MsgPushDataR(void)
	{
	}

	void fromBytes(const char*p,const int len);
	void toBytes();
};
