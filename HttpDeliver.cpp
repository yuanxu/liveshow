#include "StdAfx.h"
#include "HttpDeliver.h"
#include "sys.h"

const char* HTTP_RESPONSE =  "HTTP/1.0 200 OK\r\nServer: LiveShowServer\r\nCache-Control: no-cache\r\n\r\n";

using namespace std;
HttpDeliver::HttpDeliver(void)
{
	bIsRunning = false;
	lpHeader = NULL;

	sktListener = sys->createSocket(SOCK_STREAM);
	sys->setNoBlock(sktListener);
	/*int ioptV=1,ioptLen=sizeof(int);
	setsockopt(sktListener,SOL_SOCKET,SO_REUSEADDR,(char*)&ioptV,ioptLen);*/
	lpLock = new WLock();
	headerLen=0;
}

HttpDeliver::~HttpDeliver(void)
{
	
}


void HttpDeliver::DeliverHeader(char *p, int len)
{
	if (NULL == lpHeader)
	{
		delete[] lpHeader;
	}
	lpHeader = new char[len];
	memcpy(lpHeader,p,len);
	headerLen = len;
}

int HttpDeliver::WorkThread(ThreadInfo *Info)
{
	HttpDeliver* me = (HttpDeliver*)Info->data;
	int httpPort = 8087;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	addr.sin_port = htons(httpPort);
	while(SOCKET_ERROR == bind(me->sktListener,(sockaddr*)&addr,sizeof(addr)))
	{
#ifdef _DEBUG
		std::cout << "Http Deliver can't bind " << GetLastError();
#endif
		httpPort=HttpDeliver::randInt(5000,6000);
		addr.sin_port = htons(httpPort);
		//me->setStatus(SC_HTTPD_START_FAILED,GetLastError());
		//return -1;
	};

	if (SOCKET_ERROR == listen(me->sktListener,3))
	{
#ifdef _DEBUG
		std::cout << "Http Deliver can't start listen " << GetLastError();
#endif
		me->setStatus(SC_HTTPD_START_FAILED,GetLastError());
		return -1;
	}
	me->setStatus(SC_HTTPD_STARTED,httpPort);

	HttpSocket httpSkt ;
	httpSkt.skt = me->sktListener;
	httpSkt.bIsListener = true;
	me->vtSkts.push_back(httpSkt);

	FD_SET fd_Rds,fd_wrts;
	timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	while (me->bIsRunning)
	{
		Sleep(1);
		FD_ZERO(&fd_Rds);
		FD_ZERO(&fd_wrts);

		me->lpLock->on();
		for (vector<HttpSocket>::iterator it = me->vtSkts.begin() ; it != me->vtSkts.end() ; it++)
		{
			FD_SET((*it).skt ,&fd_Rds);
			FD_SET((*it).skt,&fd_wrts);	
		}
		me->lpLock->off();
		if (SOCKET_ERROR == select(0,&fd_Rds,&fd_wrts,NULL,&tv))
		{
#ifdef _DEBUG
			cout << "HTTP Deliver select ERROR!" << GetLastError();
#endif
		}
		vector<SOCKET> _vtSks;
		
		me->lpLock->on();
		for (vector<HttpSocket>::iterator it = me->vtSkts.begin() ; it != me->vtSkts.end() ; )
		{
			if (FD_ISSET((*it).skt,&fd_Rds))
			{
				if ((*it).bIsListener)
				{
					sockaddr_in addr;
					int addrlen = sizeof(addr);
					SOCKET skt = SOCKET_ERROR  ;
					while(me->bIsRunning && skt == SOCKET_ERROR)
					{
						skt  = accept(me->sktListener,(sockaddr*)&addr,&addrlen);
					}
					//sys->setNoBlock(skt);
					if (me->headerLen==0)
					{
						shutdown(skt,SD_BOTH);
						closesocket(skt);
					}
					else
						_vtSks.push_back(skt);
				}
				else
				{
					//客户端过来请求
					//不管是什么，直接反馈Http 200 ok
					char buf[8192];
					if (SOCKET_ERROR == recv((*it).skt,buf,sizeof(buf),0))
					{
#ifdef _DEBUG
						cout << "HTTP Deliver recv ERROR!" << GetLastError();
#endif
						if (GetLastError()!=WSAEWOULDBLOCK) 
						{
							it = me->vtSkts.erase(it);
							continue;
						}
						else
						{
							Sleep(50);
						}
						
					}
					//send 200 ok
					(*it).Deliver(HTTP_RESPONSE,strlen(HTTP_RESPONSE));
					
					//send header
					
					int ret =0;
					(*it).bIsReady=true;
					
					(*it).Deliver(me->lpHeader,me->headerLen);

					// deliver data
					for (vector<char*>::iterator it2 = me->vtBuf.begin() ; it2 != me->vtBuf.end() ; it2++)
					{
						(*it).Deliver(*it2);
					}
					(*it).bIsReady = true;
				}
				if(FD_ISSET((*it).skt,&fd_wrts))
				{
					(*it).bIsReady = true;
				}
			}
			it++;
		}

		for (vector<SOCKET>::iterator it = _vtSks.begin() ; it != _vtSks.end() ; it++)
		{
			HttpSocket hSkt ;
			hSkt.skt = (*it);
			hSkt.bIsListener =false;
			hSkt.bIsReady = false;
			sys->setNoBlock(hSkt.skt);
			me->vtSkts.push_back(hSkt);
			
		}
		
		
		me->lpLock->off();
	}
	me->lpLock->on();
	for (vector<HttpSocket>::iterator it = me->vtSkts.begin() ; it != me->vtSkts.end() ;it++)
	{
		closesocket((*it).skt);
	}
	me->lpLock->off();
	return -31;
}

int HttpDeliver::SendThread(ThreadInfo *lpInfo)
{
	HttpDeliver* me = (HttpDeliver*)lpInfo->data;
	vector<HttpSocket> vtSktT;
	while (me->bIsRunning)
	{
		Sleep(20);
		vtSktT.clear();
		me->lpLock->on();
		for (vector<HttpSocket>::iterator it = me->vtSkts.begin() ; it!= me->vtSkts.end();)
		{
			try
			{
				(*it).Send(); //处理Socket Exception
				it++;
			}
			catch (SocketException*) 
			{
				it =me->vtSkts.erase(it);
			}
		}
		

		me->lpLock->off();
	}
	return -32;
}

void HttpDeliver::Start()
{
	bPlayerStared = false;
	dwExpectSegmentID=0;
	bIsRunning = true;
	lpWorkThread = new ThreadInfo();
	lpWorkThread->func = (THREAD_FUNC)WorkThread;
	lpWorkThread->data = this;
	sys->startThread(lpWorkThread);

	lpSendThread = new ThreadInfo();
	lpSendThread->func =(THREAD_FUNC)SendThread;
	lpSendThread->data = this;
	sys->startThread(lpSendThread);
}

void HttpDeliver::Stop()
{
	headerLen =0;
	bIsRunning = false;
	for (vector<char*>::iterator it = vtBuf.begin();it!= vtBuf.end();)
	{
		delete[] *it;
		it=vtBuf.erase(it);
	}
	closesocket(sktListener);
	//sys->endThread(lpSendThread);
	//sys->endThread(lpWorkThread);
	delete lpSendThread;
	delete lpWorkThread;
}


void HttpDeliver::Deliver(Segment *pSeg, bool bIsImmediate)
{
	
	lpLock->on();
	if (NULL==pSeg || dwExpectSegmentID > pSeg->getSegID() && dwExpectSegmentID!=0) //已经Deliver过了
	{
		lpLock->off();
		return;
	}
	if (vtBuf.size() == static_cast<int>(READY_TO_PLAY) && !bPlayerStared && headerLen>0)
	{
#ifndef _WINDLL
		ShellExecute(NULL,NULL,_T("http://localhost:8087/liveshow.asf"),NULL,NULL,SW_SHOWNORMAL);
#endif
		setStatus(SC_READY_TO_PLAY,0);
		bPlayerStared = true;
	}
	if (vtBuf.size() >= static_cast<int>(MAX_CACHE_SIZE))
	{
		delete[] *vtBuf.begin();
		vtBuf.erase(vtBuf.begin());
	}
	char *lpData = NULL;
	int iSize = pSeg->getData2(&lpData,bIsImmediate);
	//iSize = Segment::getSegmentSize();
	if (NULL==lpData )
	{
		lpLock->off();
		return;
	}
	if (iSize == Segment::getSegmentSize())
	{
		vtBuf.push_back(lpData);
	}

	for(vector<HttpSocket>::iterator it = vtSkts.begin(); it != vtSkts.end();it++)
	{

		if (!(*it).bIsListener)
		{
			(*it).Deliver(lpData,iSize);
		}	
	}
	
	dwExpectSegmentID =pSeg->getSegID() +1;
	lpLock->off();
}