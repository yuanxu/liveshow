#include "stdafx.h"
#include "utils.h"

char* IniUtil::lpConfig = NULL;

WORD IniUtil::LocalPort = 8086;
DWORD IniUtil::PrxSrvIP = 0;
WORD IniUtil::PrxSrvPort = 0;
DWORD IniUtil::SrvIP = 0;
WORD IniUtil::SrvPort = 0;
DWORD IniUtil::LocalIP = 0;

DWORD IniUtil::getLocalIP()
{
	return LocalIP;
}
void IniUtil::setLocalIP(DWORD ip)
{
	 LocalIP = ip;
}
WORD IniUtil::getLocalPort()
{
#ifdef _WINDLL
	return htons(LocalPort);
#else
	IniUtil::Init();
	char buf[6];
	GetPrivateProfileStringA("local","port","8086",buf,6,lpConfig);
	return htons(static_cast<WORD>(atoi(buf)));
#endif
}

WORD IniUtil::getSourcePinPort()
{
#ifdef _WINDLL
	return htons(LocalPort);
#else
	IniUtil::Init();
	char buf[6];
	GetPrivateProfileStringA("local","sourcePinPort","8086",buf,6,lpConfig);
	return htons(static_cast<WORD>(atoi(buf)));
#endif
}

DWORD IniUtil::getServerIP()
{
#ifdef _WINDLL
	return SrvIP;
#else
	IniUtil::Init();
	char buf[16];
	DWORD ret= GetPrivateProfileStringA("proxyServer","ip","",buf,512,lpConfig);
	return inet_addr(buf);
#endif
}

WORD IniUtil::getServerPort()
{
#ifdef _WINDLL
	return htons(SrvPort);
#else
	IniUtil::Init();
	char buf[6];
	GetPrivateProfileStringA("server","port","8086",buf,6,lpConfig);
	return htons(static_cast<WORD>(atoi(buf)));
#endif
}

DWORD IniUtil::getProxySrvIP()
{
#ifdef _WINDLL
	return PrxSrvIP;
#else
	IniUtil::Init();
	char buf[16];
	DWORD ret= GetPrivateProfileStringA("proxyServer","ip","",buf,512,lpConfig);
	return inet_addr(buf);
#endif
}

WORD IniUtil::getProxySrvPort()
{
#ifdef _WINDLL
	return htons(PrxSrvPort);
#else
	IniUtil::Init();
	char buf[6];
	GetPrivateProfileStringA("proxyServer","port","8255",buf,6,lpConfig);
	return htons(static_cast<WORD>(atoi(buf)));
#endif
}

void IniUtil::setLocalPort(WORD value)
{
	LocalPort = value;
}

void IniUtil::setServerIP(DWORD value)
{
	SrvIP = value;
}

void IniUtil::setServerPort(WORD value)
{
	SrvPort = value;
}

void IniUtil::setProxySrvIP(DWORD value)
{
	PrxSrvIP = value;
}

void IniUtil::setProxySrvPort(WORD value)
{
	PrxSrvPort = value;
}