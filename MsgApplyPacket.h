#pragma once
#include "messagebase.h"

class MsgApplyPacket :
	public MessageBase
{
public:
	MsgApplyPacket(void);
	MsgApplyPacket(const char*p,const int len){fromBytes(p,len);};
	~MsgApplyPacket(void);
	
	SEGMENTID dwSegID;
	char cSeq;

	void fromBytes(const char *p,const int len);
	void toBytes();
};
