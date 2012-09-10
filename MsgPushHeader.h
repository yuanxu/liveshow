#pragma once
#include "messagebase.h"
#include <time.h>
class MsgPushHeader :
	public ReliableMessageBase
{
public:
	MsgPushHeader(void);
	~MsgPushHeader(void);

public:
	SEGMENTID wInitSegID;	//初始SegID
	time_t tInitTime;	//初始时间
	char *lpData;			//header数据
	int dataLen;			//数据长度
private:
	bool bIsOwnerBuf;
public:
	void fromBytes(const char*p,const int len);
	void toBytes();
};

//-------------------------------------------------------

class MsgPushHeaderR :
	public ACKMessageBase
{
public:
	MsgPushHeaderR(WORD wSeq):ACKMessageBase()
	{
		wCMD = CMD_PUSH_HEADER_R;
		wSeqID = wSeq;
	}
	MsgPushHeaderR(void):ACKMessageBase(){wCMD = CMD_PUSH_HEADER_R;};
public:
	void fromBytes(const char*p,const int len);
	void toBytes();
};