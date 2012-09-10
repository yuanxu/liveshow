
// ------------------------------------------------
// File : common.h
// Date: 4-apr-2002
// Author: giles
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
#pragma warning( disable : 4996 )

typedef DWORD PEERID;	//节点ID
typedef DWORD SEGMENTID; //SegmnetID

//常量定义
#define MAX_SEGMENT_COUNT  64		// Segmnet中最大包数
#define BM_SIZE	(MAX_SEGMENT_COUNT /8 + sizeof(SEGMENTID))		//Buffer Map长度 7.5+2	。必须确保MAX_SEGMENT_COUNT%8 ==0
#define PACKET_SIZE  8192		// 数据传输中，数据包大小.1450+28(UDP header size) = 1478
#define PTL_HEADER_SIZE			10		//协议消息头长度
#define PACKET_BODY_SIZE	(PACKET_SIZE - PTL_HEADER_SIZE)
#define PACKET_DATA_SIZE  (PACKET_BODY_SIZE- sizeof(SEGMENTID) -1)// 数据传输中，数据块大小。8kb - segID(4) -seq(1)
#define MAX_LIVE_TIME  MAX_SEGMENT_COUNT		// Segmment的最大存活时间，单位秒

//#define MAX_RESEND_TIME	3			//消息最大重发次数
//#define	RESEND_INTERVAL	5			//重发间隔，单位：秒
#define STARTID_MARGIN	0.1				//10%

//-----------状态码-----------
extern "C"
{
#define SC_INIT_STARTED 10 //开始初始化
#define SC_INIT_FINISHED 11 //初始化完成
#define SC_INIT_FAILED	12 //初始化错误
#define SC_CONNECT_STARTED 13 //开始连接服务器
#define SC_CONNECT_FAILED 14 //连接服务器失败
#define SC_CONNECT_OK	15	//连接服务器成功

#define SC_CMD_GETHEAD	20	//得到了媒体头
#define SC_BUFFER	21	//缓冲状态
#define SC_READY_TO_PLAY	22	//可以播放

#define SC_HTTPD_STARTED 30	//http服务已经开始
#define SC_HTTPD_START_FAILED 31 //http服务启动失败
#define SC_HTTPD_STOPED	32	//http服务已停止
};

#if _BIG_ENDIAN
#define CHECK_ENDIAN2(v) v=SWAP2(v)
#define CHECK_ENDIAN3(v) v=SWAP3(v)
#define CHECK_ENDIAN4(v) v=SWAP4(v)
#else
#define CHECK_ENDIAN2
#define CHECK_ENDIAN3
#define CHECK_ENDIAN4
#endif

//-----------------------------

#define SWAP2(v) ( ((v&0xff)<<8) | ((v&0xff00)>>8) )
#define SWAP3(v) (((v&0xff)<<16) | ((v&0xff00)) | ((v&0xff0000)>>16) )
#define SWAP4(v) (((v&0xff)<<24) | ((v&0xff00)<<8) | ((v&0xff0000)>>8) | ((v&0xff000000)>>24))
#define TOUPPER(c) ((((c) >= 'a') && ((c) <= 'z')) ? (c)+'A'-'a' : (c))
#define TONIBBLE(c) ((((c) >= 'A')&&((c) <= 'F')) ? (((c)-'A')+10) : ((c)-'0'))
#define BYTES_TO_KBPS(n) (float)(((((float)n)*8.0f)/1024.0f))


// ----------------------------------
class GeneralException
{
public:
    GeneralException(const char *m, int e = 0) 
	{
		strcpy_s(msg,m);
		err=e;
	}
    char msg[128];
	int err;
};

// -------------------------------------
class StreamException : public GeneralException
{
public:
	StreamException(const char *m) : GeneralException(m) {}
	StreamException(const char *m,int e) : GeneralException(m,e) {}
};

// ----------------------------------
class SockException : public StreamException
{
public:
    SockException(const char *m="Socket") : StreamException(m) {}
    SockException(const char *m, int e) : StreamException(m,e) {}
};

//////////////////////////////////////////////////////////////////////////
class SocketException
{
public:
	UINT	dwError;
	SocketException(DWORD err)//:dwError(err)
	{
		dwError = err;
	};

};

// ----------------------------------
class EOFException : public StreamException
{
public:
    EOFException(const char *m="EOF") : StreamException(m) {}
    EOFException(const char *m, int e) : StreamException(m,e) {}
};

// ----------------------------------
class CryptException : public StreamException
{
public:
    CryptException(const char *m="Crypt") : StreamException(m) {}
    CryptException(const char *m, int e) : StreamException(m,e) {}
};


// ----------------------------------
class TimeoutException : public StreamException
{
public:
    TimeoutException(const char *m="Timeout") : StreamException(m) {}
};


// ----------------------------------
class Host
{
    inline unsigned int ip3()
    {
        return (ip>>24);
    }
    inline unsigned int ip2()
    {
        return (ip>>16)&0xff;
    }
    inline unsigned int ip1()
    {
        return (ip>>8)&0xff;
    }
    inline unsigned int ip0()
    {
        return ip&0xff;
    }

public:
	Host(){init();}
	Host(unsigned int i, unsigned short p)
	{
		ip = i;
		port = p;
		value = 0;
	}

	void	init()
	{
		ip = 0;
		port = 0;
		value = 0;
	}


	bool	isMemberOf(Host &);

	bool	isSame(Host &h)
	{
		return (h.ip == ip) && (h.port == port);
	}

	bool classType() {return globalIP();}

	bool	globalIP()
	{
		// local host
		if ((ip3() == 127) && (ip2() == 0) && (ip1() == 0) && (ip0() == 1))
			return false;

		// class A
		if (ip3() == 10)
			return false;

		// class B
		if ((ip3() == 172) && (ip2() >= 16) && (ip2() <= 31))
			return false;

		// class C
		if ((ip3() == 192) && (ip2() == 168))
			return false;

		return true;
	}
	bool	localIP()
	{
		return !globalIP();
	}

	bool	loopbackIP()
	{
//		return ((ipByte[3] == 127) && (ipByte[2] == 0) && (ipByte[1] == 0) && (ipByte[0] == 1));
		return ((ip3() == 127) && (ip2() == 0) && (ip1() == 0) && (ip0() == 1));
	}

	bool	isValid()
	{
		return (ip != 0);
	}


	bool	isSameType(Host &h)
	{
			return ( (globalIP() && h.globalIP()) ||
			         (!globalIP() && !h.globalIP()) ); 
	}

	void	IPtoStr(char *str)
	{
		sprintf(str,"%d.%d.%d.%d",(ip>>24)&0xff,(ip>>16)&0xff,(ip>>8)&0xff,(ip)&0xff);
	}

	void	toStr(char *str)
	{
		sprintf(str,"%d.%d.%d.%d:%d",(ip>>24)&0xff,(ip>>16)&0xff,(ip>>8)&0xff,(ip)&0xff,port);
	}

	void	fromStrIP(const char *,int);
	void	fromStrName(const char *,int);

	bool	isLocalhost();


	union
	{
		unsigned int ip;
//		unsigned char ipByte[4];
	};

    unsigned short port;
	unsigned int value;
};
// ----------------------------------
#define SWAP2(v) ( ((v&0xff)<<8) | ((v&0xff00)>>8) )
#define SWAP3(v) (((v&0xff)<<16) | ((v&0xff00)) | ((v&0xff0000)>>16) )
#define SWAP4(v) (((v&0xff)<<24) | ((v&0xff00)<<8) | ((v&0xff0000)>>8) | ((v&0xff000000)>>24))
#define TOUPPER(c) ((((c) >= 'a') && ((c) <= 'z')) ? (c)+'A'-'a' : (c))
#define TONIBBLE(c) ((((c) >= 'A')&&((c) <= 'F')) ? (((c)-'A')+10) : ((c)-'0'))
#define BYTES_TO_KBPS(n) (float)(((((float)n)*8.0f)/1024.0f))

// ----------------------------------
inline bool isWhiteSpace(char c)
{
	return (c == ' ') || (c == '\r') || (c == '\n') || (c == '\t');
};

inline char * memfind(char *buff,int buffsize,char *sub,int subsize)
{
	char *p=buff;
	while(p<buff+buffsize)
	{
		if(*p++!=*sub) continue;
		if(!memcmp(--p,sub,subsize))
			return p;
		p++;
	}
	return NULL;
}