#pragma once
#include "stdafx.h"
#include "sys.h"

#define CONFIG_FILE "\\config.ini" //≈‰÷√Œƒº˛√˚

class IniUtil
{
private:
	static char *lpConfig ;
	static DWORD PrxSrvIP,SrvIP,LocalIP;
	static WORD PrxSrvPort,SrvPort,LocalPort;
public:
	static void Init()
	{
#ifndef _WINDLL
		if(NULL != lpConfig)
			return;

		char buf[512] ;
		DWORD ret = GetCurrentDirectoryA(sizeof(buf),buf);
		int ilen =static_cast<int>( strlen(CONFIG_FILE));
		lpConfig = new char[ret + ilen +1];
		memset(lpConfig,0,ret + ilen +1);
		memcpy(lpConfig,buf,ret);
		memcpy(lpConfig + ret,CONFIG_FILE,ilen);
#endif
	};
	
	static DWORD getLocalIP();
	static WORD getLocalPort();
	
	static DWORD getServerIP();
	static WORD getServerPort();

	static DWORD getProxySrvIP();
	static WORD  getProxySrvPort();
	
	static WORD getSourcePinPort();

	static void setLocalIP(DWORD ip);
	static void setLocalPort(WORD value);

	static void setServerIP(DWORD value);
	static void setServerPort(WORD value);

	static void setProxySrvIP(DWORD value);
	static void  setProxySrvPort(WORD value);
};