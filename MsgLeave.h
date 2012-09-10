#pragma once
#include "messagebase.h"

class MsgLeave :
	public MessageBase
{
public:
	MsgLeave(void);
	~MsgLeave(void);

public:
	void fromBytes(const char* p ,const int len);
	void toBytes();
};

//-----------------------------------------------------

class MsgLeaveR:
	public MsgLeave
{
public:
	MsgLeaveR():MsgLeave(){};
	~MsgLeaveR(){};
};