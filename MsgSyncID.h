#pragma once
#include "messagebase.h"

class MsgSyncID :
	public ReliableMessageBase
{

public:
	MsgSyncID(void);
	~MsgSyncID(void);
	PEERID dwSID;
	void fromBytes(const char*p,const int len);
	void toBytes();
};

class MsgSyncIDR :
	public ACKMessageBase
{
public:
	SEGMENTID dwSegID;
	MsgSyncIDR(void);
	~MsgSyncIDR(void);
	void fromBytes(const char*p,const int len);
	void toBytes();
};