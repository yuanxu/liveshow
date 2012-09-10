// LiveShowEx1.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
//#include "wme_c.h"
#include "ServntMgr.h"
#include "utils.h"
#ifdef _WINDLL
#include "export.h"
BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved)
{
	switch( ul_reason_for_call ) {
	case DLL_PROCESS_ATTACH:
		{
			WSADATA wsaData;
			int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
			if (iResult != NO_ERROR)
				printf("Error at WSAStartup()\n");

			lpMgr = new ServntMgr();
		}
		break;
	case DLL_THREAD_ATTACH:
		
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		
			//lpMgr->Stop();
			delete lpMgr;
		
		
		break;
	}
	return TRUE;
}

#else

int _tmain(int argc, _TCHAR* argv[])
{

  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != NO_ERROR)
    printf("Error at WSAStartup()\n");
  cout << "Please choice the program's role. \r\n[s]erver or [c]lient: ";
	char r ;
	ServntMgr *lpMgr = new ServntMgr();
	
	//wme_c *wme = new wme_c();
	bool bStarted = false;
	char lpSrvIP[15];// = NULL;
	while(1)
	{
		cin >> r;
		switch(r)
		{
		case 's':
			/*ThreadInfo *msgPool = new ThreadInfo();
			msgPool->func =(THREAD_FUNC) MsgPool;
			sys->startThread(msgPool);
			*/
			if(bStarted)
				continue;
			lpMgr->srv_Start(1);
			bStarted = true;
			//wme->Start("192.168.21.40",8089);
			break;
		case 'c':
			//wme->Stop();
			{

			
			if(bStarted)
				continue;
			in_addr addr;
			addr.s_addr = IniUtil::getServerIP();
			char* lpTmp =inet_ntoa(addr);
			//lpSrvIP = new char(sizeof(lpTmp));
			strcpy(lpSrvIP,lpTmp);
			lpMgr->Start(lpSrvIP ,8086,1);
			bStarted = true;
			}
			return 0;
		case 'e':
			lpMgr->Stop();
		case 'q':
			break;
		}
	}
	return 0;
}
#endif