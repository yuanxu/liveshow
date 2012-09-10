// ------------------------------------------------
// File : sys.h
// Date: 4-apr-2002
// Author: giles
// Desc: 
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
#pragma once

#ifndef _SYS_H
#define _SYS_H

#include <string.h>
#include <windows.h>
#include "common.h"


#define RAND(a,b) (((a = 36969 * (a & 65535) + (a >> 16)) << 16) + \
                    (b = 18000 * (b & 65535) + (b >> 16))  )
extern char *stristr(const char *s1, const char *s2);
extern char *trimstr(char *s);

#define MAX_CGI_LEN 512

// ------------------------------------
class Sys
{
public:
	Sys();



	virtual bool			startThread(class ThreadInfo *) = 0;
	virtual void			sleep(int) = 0;
	virtual void			appMsg(long,long = 0) = 0;
	virtual unsigned int	getTime() = 0;		
	virtual unsigned int	rnd() = 0;
	virtual void			getURL(const char *) = 0;

	virtual void			callLocalURL(const char *,int)=0;
	virtual void			executeFile(const char *) = 0;
	virtual void			endThread(ThreadInfo *) {}
	virtual void			waitThread(ThreadInfo *, int timeout = 30000) {}
	virtual SOCKET			createSocket(int) =0;
	virtual	void			setNoBlock(SOCKET skt) =0;

#ifdef __BIG_ENDIAN__
	unsigned short	convertEndian(unsigned short v) { return SWAP2(v); }
	unsigned int	convertEndian(unsigned int v) { return SWAP4(v); }
#else
	unsigned short	convertEndian(unsigned short v) { return v; }
	unsigned int	convertEndian(unsigned int v) { return v; }
#endif


	void	sleepIdle();

	unsigned int idleSleepTime;
	unsigned int rndSeed;
	unsigned int numThreads;
	
};


#ifdef WIN32
#include <windows.h>

typedef __int64 int64_t;


// ------------------------------------
class WEvent
{
public:
	WEvent()
	{
		 event = ::CreateEvent(NULL, // no security attributes
                                  TRUE, // manual-reset
                                  FALSE,// initially non-signaled
                                  NULL);// anonymous
	}

	~WEvent()
	{
		::CloseHandle(event);
	}

	void	signal()
	{
		::SetEvent(event);
	}

	void	wait(int timeout = 30000)
	{
		switch(::WaitForSingleObject(event, timeout))
		{
          case WAIT_TIMEOUT:
              throw TimeoutException();
              break;
          //case WAIT_OBJECT_0:
              //break;
		}
	}

	void	reset()
	{
		::ResetEvent(event);
	}



	HANDLE event;
};



// ------------------------------------
typedef int (WINAPI *THREAD_FUNC)(ThreadInfo *);
typedef unsigned int THREAD_HANDLE;
#define THREAD_PROC int WINAPI
#define vsnprintf _vsnprintf

// ------------------------------------
class WLock
{
public:
	WLock()
	{
		InitializeCriticalSection(&cs);
	}


	void	on()
	{
		EnterCriticalSection(&cs);
	}

	void	off()
	{
		LeaveCriticalSection(&cs);
	}
	
	CRITICAL_SECTION cs;
};
#endif



// ------------------------------------
class ThreadInfo
{
public:
	//typedef int  (__stdcall *THREAD_FUNC)(ThreadInfo *);

	ThreadInfo()
	{
		active = false;
		id = 0;
		func = NULL;
		data = NULL;
		 isWorkFuncThread = false;
	}

	void	shutdown();

	volatile bool	 active;
	int		id;
	THREAD_FUNC func;
	THREAD_HANDLE handle;
	bool isWorkFuncThread;

	inline bool setPriority(int nPriority)
	{
		BOOL ret = SetThreadPriority((HANDLE)handle,nPriority);
		if(ret == 0)
		{
			DWORD dwErr = GetLastError();
			dwErr=dwErr;
		}
		return ret != 0;
	};

	void	*data;
};

// ------------------------------------

extern Sys *sys;

// ------------------------------------


#if _BIG_ENDIAN
#define CHECK_ENDIAN2(v) v=SWAP2(v)
#define CHECK_ENDIAN3(v) v=SWAP3(v)
#define CHECK_ENDIAN4(v) v=SWAP4(v)
#else
#define CHECK_ENDIAN2
#define CHECK_ENDIAN3
#define CHECK_ENDIAN4
#endif


// ------------------------------------
#endif


// ------------------------------------
class WSys : public Sys
{
public:
	WSys();
	~WSys();

	 bool			startThread(ThreadInfo *);
	 void			sleep(int );
	 void			appMsg(long,long);
	 unsigned int	getTime();

	 unsigned int	rnd() {return 0;}
	 void			getURL(const char *);
	//virtual bool			hasGUI() {return mainWindow!=NULL;}
	 void			callLocalURL(const char *str,int port);
	 void			executeFile(const char *);
	 void			endThread(ThreadInfo *);
	 void			waitThread(ThreadInfo *, int timeout = 30000);
	SOCKET			createSocket( int);
	void			setNoBlock(SOCKET skt);



//	peercast::Random rndGen;
};                               

