/************************************************************************/
/* 这里定义导出函数                                                     */
/************************************************************************/
#include "ServntMgr.h"
#include "winsock.h"
ServntMgr *lpMgr = NULL;
extern "C"
{
	//启动
	_declspec(dllexport)  void __stdcall Start(char *szIP, short wPort, int ChID)
	{
		IniUtil::setServerIP( inet_addr(szIP));
		IniUtil::setServerPort(wPort);
		lpMgr->Start(szIP,wPort,ChID);
	};

	//停止
	_declspec(dllexport) void __stdcall Stop()
	{
		lpMgr->Stop();
	};
	
	//设置接收状态的接口
	_declspec(dllexport) void __stdcall SetStatusPin(LPVOID lpPin)
	{
		lpMgr->StatusPin = reinterpret_cast<void(__stdcall*)(int,int)>(lpPin);
	}

	_declspec(dllexport) void __stdcall SetProxyServer(char* szIP,short wPort)
	{
		IniUtil::setProxySrvIP( inet_addr(szIP));
		IniUtil::setProxySrvPort(wPort);
	}

	
};