#pragma once
#include "messagebase.h"

class MsgForward :
	public MessageBase
{
public:
	MsgForward(void);
	MsgForward(const char* p,const int len) 
	{
		fromBytes(p,len);
	};
	~MsgForward(void);

public:
	//请求连接的客户标识
	DWORD	dwSID;
	//版本
	WORD	wVersion;
	DWORD	dwIP;
	WORD	wPort;
	char*	szChID;

public:
	void fromBytes(const char* p,const int len);
	void toBytes();
};

//-------------------------------------------------------------------------------------

class MsgForwardR
	:public MessageBase
{
public:
	char Result;

public:
	MsgForwardR();
	~MsgForwardR();
	void fromBytes(const char *p,const int len);
	void toBytes();
};

