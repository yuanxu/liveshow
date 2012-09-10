#pragma once
#include "MessageBase.h"
#include <vector>

using namespace std;

typedef struct tagUdpCmd
{
	//指令
	WORD wCMD;
	//发送指令的SID
	DWORD dwSID;
	//发送指令者的IP
	DWORD dwIP;
	//发送指令者的端口号
	WORD wPort;
	//对应的频道ID
	DWORD	CHID;
}	UdpCmd;
#define SizeOfUdpCmd 16 
//////////////////////////////////////////////////////////////////////////

class UdpHello
	: public MessageBase
{
public:
	UdpHello():MessageBase()
	{
		wCMD = PXY_HELLO;
	};
	~UdpHello()
	{
		
	}
	DWORD SID;
	void fromBytes(const char*p,const int len);
	void toBytes();
};

class UdpHelloR
	: public MessageBase
{
public:
	UdpHelloR():MessageBase()
	{
		wCMD = PXY_HELLO_R;
	};
	//接收指令者
	DWORD SID;
	vector<UdpCmd*> vtCmds;	//一般存放的是callback请求
	void fromBytes(const char*p,const int len);
	void toBytes();
};

//////////////////////////////////////////////////////////////////////////

class UdpCallBack
	:public MessageBase
{
public:
	UdpCallBack():MessageBase()
	{
		wCMD = PXY_CALLBACK;
	}
	~UdpCallBack()
	{
		
	}
	//接收指令者s
	DWORD SID;
	UdpCmd Body;

	void fromBytes(const char*p,const int len);
	void toBytes();
};

