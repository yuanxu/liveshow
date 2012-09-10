#include "StdAfx.h"
#include "SourcePin.h"
#include "sys.h"
#include "asf.h"
#include "utils.h"
SourcePin::SourcePin(void)
{
	bIsRunning = false;
}

SourcePin::~SourcePin(void)
{
}

void SourcePin::Start(Buffer *lpBuf)
{
	lpThrd = new ThreadInfo();
	lpThrd->data = this;
	this->lpBuf = lpBuf;
	lpThrd->func = (THREAD_FUNC)DataProcessFunc;
	sys->startThread(lpThrd);
}

int SourcePin::DataProcessFunc(ThreadInfo *lpInfo)
{
	SourcePin *me = (SourcePin*)lpInfo->data;
	me->bIsRunning = true;
	SOCKET sktListen;
	sktListen = sys->createSocket(SOCK_STREAM);
	sockaddr_in addr;
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = IniUtil::getSourcePinPort();
	if(SOCKET_ERROR == bind(sktListen,(sockaddr*)&addr,sizeof(addr)))
	{
#ifdef _DEBUG
		std::cout << "SourcePin can't bind " << GetLastError();
#endif
		return -51;
	};

	if (SOCKET_ERROR == listen(sktListen,1))
	{
#ifdef _DEBUG
		std::cout << "Http Deliver can't start listen " << GetLastError();
#endif
		return -52;
	}
	char *buf = new char [512*1024];//1MB
	while(me->bIsRunning)
	{
		sockaddr_in addr_r;
		int addrLen = sizeof(addr_r);
		SOCKET skt = accept(sktListen,(sockaddr*)&addr_r,&addrLen);
		if (skt==INVALID_SOCKET)
		{
			Sleep(5);
			continue;
		}
		while (me->bIsRunning)
		{
			Sleep(1);
			memset(buf,0,sizeof(buf));
			int iOffset = 0,ret =0;
			iOffset=0;
			while (iOffset!= 14)
			{
				ret = recv(skt,buf+iOffset,14-iOffset,0);
				if(ret == 0 || ret == SOCKET_ERROR)
				{
					ret = 0;
					shutdown(skt,SD_BOTH);
					closesocket(skt);
					break;
				}
				iOffset+=ret;
				Sleep(5);
			}
			if (ret ==0)
			{
				break;
			}
			DWORD dwlen,dwChID;
			char cDataType;
			memcpy(&dwlen,buf+5,4);
			memcpy(&dwChID,buf+9,4);
			cDataType = buf[13];
			iOffset= 0;
			dwlen -= 14;//
			memset(buf,0,14);
			while(iOffset!=dwlen && me->bIsRunning)
			{
				ret = recv(skt,buf+iOffset,dwlen-iOffset,0);
				if(ret == 0 || ret == SOCKET_ERROR)
				{
					ret = 0;
					DWORD dwErr = GetLastError();
					shutdown(skt,SD_BOTH);
					closesocket(skt);
					break;
				}
				iOffset+=ret;
				Sleep(5);
			}
			if (ret ==0)
			{
				break;
			}
			//处理数据
			switch(cDataType)
			{
			case 0://header
				{
					me->lpBuf->m_Header.len = dwlen;
					me->lpBuf->m_Header.lpData = new char[dwlen];
					memcpy(me->lpBuf->m_Header.lpData,buf,dwlen);
					asfHeader *objHeader = new asfHeader(buf,dwlen);
					DWORD dwPacketSize = objHeader->getChunkSize();
					DWORD dwBitRate = objHeader->getBitRate();
					delete objHeader;
					int size = ( dwBitRate /8 /dwPacketSize)*dwPacketSize;//(349056 /8 /0x5a4) * 0x5a4;
					Segment::setSegmentSize( size);
					Segment::setChunkSize(dwPacketSize);
					me->lpBuf->Init(0);
				}
				break;
			case 1://middle of data
			case 2://end of data
				static PEERID segID=0;
				me->lpBuf->srv_PushData(segID,buf,dwlen);
				segID++;
				break;
			}
		}
	}
	delete[] buf;
	return -51;
}
void SourcePin::Stop()
{
	bIsRunning = false;
}