// ------------------------------------------------
// File : sys.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		Sys is a base class for all things systemy, like starting threads, creating sockets etc..
//		Lock is a very basic cross platform CriticalSection class		
//		SJIS-UTF8 conversion by ????
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#include "stdafx.h"
#include "common.h"
#include "sys.h"
#include <stdlib.h>
#include <time.h>
#include <process.h>

// -----------------------------------
#define isSJIS(a,b) ((a >= 0x81 && a <= 0x9f || a >= 0xe0 && a<=0xfc) && (b >= 0x40 && b <= 0x7e || b >= 0x80 && b<=0xfc))
#define isEUC(a) (a >= 0xa1 && a <= 0xfe)
#define isASCII(a) (a <= 0x7f) 
#define isPLAINASCII(a) (((a >= '0') && (a <= '9')) || ((a >= 'a') && (a <= 'z')) || ((a >= 'A') && (a <= 'Z')))
#define isUTF8(a,b) ((a & 0xc0) == 0xc0 && (b & 0x80) == 0x80 )
#define isESCAPE(a,b) ((a == '&') && (b == '#'))
#define isHTMLSPECIAL(a) ((a == '&') || (a == '\"') || (a == '\'') || (a == '<') || (a == '>'))



// -----------------------------------
// base64 encode/decode taken from ices2 source.. 
static char base64table[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};
#if 0
// -----------------------------------
static char *util_base64_encode(char *data)
{
    int len = strlen(data);
    char *out = malloc(len*4/3 + 4);
    char *result = out;
    int chunk;

    while(len > 0) {
        chunk = (len >3)?3:len;
        *out++ = base64table[(*data & 0xFC)>>2];
        *out++ = base64table[((*data & 0x03)<<4) | ((*(data+1) & 0xF0) >> 4)];
        switch(chunk) {
            case 3:
                *out++ = base64table[((*(data+1) & 0x0F)<<2) | ((*(data+2) & 0xC0)>>6)];
                *out++ = base64table[(*(data+2)) & 0x3F];
                break;
            case 2:
                *out++ = base64table[((*(data+1) & 0x0F)<<2)];
                *out++ = '=';
                break;
            case 1:
                *out++ = '=';
                *out++ = '=';
                break;
        }
        data += chunk;
        len -= chunk;
    }

    return result;
}
#endif

// -----------------------------------
static int base64chartoval(char input)
{
    if(input >= 'A' && input <= 'Z')
        return input - 'A';
    else if(input >= 'a' && input <= 'z')
        return input - 'a' + 26;
    else if(input >= '0' && input <= '9')
        return input - '0' + 52;
    else if(input == '+')
        return 62;
    else if(input == '/')
        return 63;
    else if(input == '=')
        return -1;
    else
        return -2;
}

// -----------------------------------
static char *util_base64_decode(char *input)
{
	return NULL;
}





// ------------------------------------------
Sys::Sys()
{
	idleSleepTime = 10;

	numThreads=0;
}

// ------------------------------------------
void Sys::sleepIdle()
{
	sleep(idleSleepTime);
}

// ------------------------------------------
bool Host::isLocalhost()
{
	return loopbackIP() ;//|| (ip == ClientSocket::getIP(NULL)); 
}
// ------------------------------------------
void Host::fromStrName(const char *str, int p)
{
	if (!strlen(str))
	{
		port = 0;
		ip = 0;
		return;
	}

	char name[128];
	strncpy_s(name,str,sizeof(name)-1);
	port = p;
	char *pp = strstr(name,":");
	if (pp)
	{
		port = atoi(pp+1);
		pp[0] = 0;
	}

	//ip = ClientSocket::getIP(name);
}
// ------------------------------------------
void Host::fromStrIP(const char *str, int p)
{
	unsigned int ipb[4];
	unsigned int ipp;


	if (strstr(str,":"))
	{
		if (sscanf_s(str,"%03d.%03d.%03d.%03d:%d",&ipb[0],&ipb[1],&ipb[2],&ipb[3],&ipp) == 5)
		{
			ip = ((ipb[0]&0xff) << 24) | ((ipb[1]&0xff) << 16) | ((ipb[2]&0xff) << 8) | ((ipb[3]&0xff));
			port = ipp;
		}else
		{
			ip = 0;
			port = 0;
		}
	}else{
		port = p;
		if (sscanf_s(str,"%03d.%03d.%03d.%03d",&ipb[0],&ipb[1],&ipb[2],&ipb[3]) == 4)
			ip = ((ipb[0]&0xff) << 24) | ((ipb[1]&0xff) << 16) | ((ipb[2]&0xff) << 8) | ((ipb[3]&0xff));
		else
			ip = 0;
	}
}
// -----------------------------------
bool Host::isMemberOf(Host &h)
{
	if (h.ip==0)
		return false;

    if( h.ip0() != 255 && ip0() != h.ip0() )
        return false;
    if( h.ip1() != 255 && ip1() != h.ip1() )
        return false;
    if( h.ip2() != 255 && ip2() != h.ip2() )
        return false;
    if( h.ip3() != 255 && ip3() != h.ip3() )
        return false;

/* removed for endieness compatibility
	for(int i=0; i<4; i++)
		if (h.ipByte[i] != 255)
			if (ipByte[i] != h.ipByte[i])
				return false;
*/
	return true;
}

// -----------------------------------
char *trimstr(char *s1)
{
	while (*s1)
	{
		if ((*s1 == ' ') || (*s1 == '\t'))
			s1++;
		else
			break;

	}

	char *s = s1;

	s1 = s1+strlen(s1);

	while (*--s1)
		if ((*s1 != ' ') && (*s1 != '\t'))
			break;

	s1[1] = 0;

	return s;
}

// -----------------------------------
char *stristr(const char *s1, const char *s2)
{
	while (*s1)
	{
		if (TOUPPER(*s1) == TOUPPER(*s2))
		{
			const char *c1 = s1;
			const char *c2 = s2;

			while (*c1 && *c2)
			{
				if (TOUPPER(*c1) != TOUPPER(*c2))
					break;
				c1++;
				c2++;
			}
			if (*c2==0)
				return (char *)s1;
		}

		s1++;
	}
	return NULL;
}


// ---------------------------
void	ThreadInfo::shutdown()
{
	active = false;
	//sys->waitThread(this);
}




// ---------------------------------
WSys::WSys()
{
/*	
	WORD wVersionRequested;
WSADATA wsaData;
int err;
 
wVersionRequested = MAKEWORD( 2, 2 );
 
err = WSAStartup( wVersionRequested, &wsaData );
if ( err != 0 ) {
   
    return;
}
 

 
if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) {
    
    WSACleanup( );
    return; 
}
 
*/

}

// ---------------------------------
unsigned int WSys::getTime()
{
	time_t ltime;
	time( &ltime );
	return ltime;
}


// ---------------------------------
void WSys::endThread(ThreadInfo *info)
{
	_endthreadex(info->handle);
}             
// ---------------------------------
void WSys::waitThread(ThreadInfo *info, int timeout)
{
	switch(WaitForSingleObject((void *)info->handle, timeout))
	{
      case WAIT_TIMEOUT:
          throw TimeoutException();
          break;
	}
}
  

// ---------------------------------
bool	WSys::startThread(ThreadInfo *info)
{
	info->active = true;

	typedef unsigned ( __stdcall *start_address )( void * );

	unsigned int threadID;
	info->handle = (unsigned int)_beginthreadex ( NULL, 0, (start_address)info->func, info, 0, &threadID );
	
    if(info->handle == 0) 
		return false;

	return true;

}
// ---------------------------------
void	WSys::sleep(int ms)
{
	Sleep(ms);
}

// ---------------------------------
void WSys::appMsg(long msg, long arg)
{
	//SendMessage(mainWindow,WM_USER,(WPARAM)msg,(LPARAM)arg);
}

// --------------------------------------------------
void WSys::callLocalURL(const char *str,int port)
{
	char cmd[512];
	sprintf(cmd,"http://localhost:%d/%s",port,str);
	ShellExecuteA(NULL, NULL, cmd, NULL, NULL, SW_SHOWNORMAL);
}

// ---------------------------------
void WSys::getURL(const char *url)
{
	if (_strnicmp(url,"http://",7) || _strnicmp(url,"mailto:",7))
			ShellExecuteA(NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL);
}
// ---------------------------------

// --------------------------------------------------
void WSys::executeFile(const char *file)
{
    ShellExecuteA(NULL,"open",file,NULL,NULL,SW_SHOWNORMAL);  
}
SOCKET WSys::createSocket(int  type)
{
	int protocol ;
	if(SOCK_STREAM == type)
	{
		protocol = IPPROTO_TCP;
		type = SOCK_STREAM;
		
	}
	else
	{
		protocol = IPPROTO_UDP;
		type  = SOCK_DGRAM;
	}
	SOCKET skt = socket(AF_INET,type,protocol);

	return skt;
}
void WSys::setNoBlock(SOCKET skt)
{
	//设置为非阻塞模式
	unsigned long ul =1;
	int nRet = ioctlsocket(skt,FIONBIO,(unsigned long*)&ul);
	if (SOCKET_ERROR == nRet)
	{
		DWORD dwErr = GetLastError();
#ifdef _DEBUG
		std::cout << "Can't set to no-blocking model" << dwErr << std::endl;
#endif
	}

}
Sys* sys = new WSys();