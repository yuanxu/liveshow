#include "StdAfx.h"
#include "ServntMgr.h"
#include "Segment.h"
#include "wme_c.h"
#include "utils.h"
#include "asf.h"
#include "SourcePin.h"


//ServntMgr::StatusPin = NULL;

ServntMgr::ServntMgr(void)
{
	srand(static_cast<unsigned int>( time(0))); //保证每次运行产生的随机数都不一样
	
	bRunAsTCP = true;

	bIsSrv=false;
	lpSvt = new ServantList();
	bIsRunning = false;
	
	lpCacheLock = new WLock();
	lpThreadsLock = new WLock();
	uintPrxHello =0;
	StatusPin = NULL;
	lpHttpDeliver = NULL;
}

ServntMgr::~ServntMgr(void)
{
	StatusPin = NULL;
	if (lpHttpDeliver !=NULL)
	{
		delete lpHttpDeliver;
	}
	delete lpCacheLock;
	delete lpThreadsLock;
	delete lpSvt;
}

void ServntMgr::initMgr()
{

	setStatus(SC_INIT_STARTED);

	char lpHostName[512];
	gethostname(lpHostName,sizeof(lpHostName));

#ifdef _DEBUG
	cout << "Host Name:" << lpHostName << endl;
#endif

	hostent *lpHostEnts = gethostbyname(lpHostName);
	if(NULL == lpHostEnts)
	{
#ifdef _DEBUG
	cout << "Can't get local ips!" << GetLastError() << endl;
#endif
		setStatus(SC_INIT_FAILED,GetLastError());
	}
	else
	{
		int i =0;
		in_addr inaddr_t;
		while (lpHostEnts->h_addr_list[i])
		{
			memcpy(&inaddr_t,lpHostEnts->h_addr_list[i],lpHostEnts->h_length);
			vtLocalIps.push_back(inaddr_t.s_addr);
			
			if (IniUtil::getLocalIP()==0 )
			{
				IniUtil::setLocalIP(inaddr_t.s_addr);
			}
#ifdef _DEBUG
			cout<< "IP:" << inet_ntoa(inaddr_t) <<endl;
#endif // _DEBUG
			i++;
		}

	}
#ifdef _DEBUG
	cout << "Resoved IPs." << endl;
#endif // _DEBUG
	lpBuf = new Buffer();

	
	if (!bIsSrv)
	{

	/*	lpFileDeliver = new FileDeliver();
		lpBuf->RegisterDeliver(lpFileDeliver);*/

		lpHttpDeliver = new HttpDeliver();
		lpHttpDeliver->StatusPin = StatusPin;
		lpBuf->RegisterDeliver(lpHttpDeliver);

		lpHttpDeliver->Start();
	}

	lpBuf->bIsSrv = bIsSrv;
			 
	ThreadData *lpData = new ThreadData(this);
	lpData->bRunAsTCP = this->bRunAsTCP;
	
	if (bRunAsTCP)
	{
		m_skt = sys->createSocket(SOCK_STREAM);
		sys->setNoBlock(m_skt);
	}
	else
	{
		m_skt = sys->createSocket(SOCK_DGRAM);
	}
	
	m_UdpSkt = sys->createSocket(SOCK_DGRAM);//udp socket
	sys->setNoBlock(m_UdpSkt);

	int ioptV = 1*1024*1024; 
	int ioptLen = sizeof(int);
	setsockopt(m_skt,SOL_SOCKET,SO_RCVBUF  ,(char*)&ioptV,ioptLen);
	ioptV=512*1024;//512KB
	setsockopt(m_skt,SOL_SOCKET,SO_SNDBUF ,(char*)&ioptV,ioptLen);
//	lpData->Append(m_skt,true);
	/*ioptV=1;
	int ret=setsockopt(m_skt,SOL_SOCKET,SO_REUSEADDR,(char*)&ioptV,ioptLen);
	assert (ret==0);*/
	sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = IniUtil::getLocalPort();
	addr.sin_addr.s_addr  = htonl(INADDR_ANY);
	memset(&(addr.sin_zero), '\0', 8);
	WORD iPort = ntohs(IniUtil::getLocalPort());
	while(1)
	{
		if(SOCKET_ERROR == bind(m_skt,(sockaddr*)&addr,sizeof(addr)))
		{
			//log::WriteLine("Can't Bind");
	#ifdef _DEBUG
			std::cout << "Can't Bind " <<  GetLastError();
	#endif
			//setStatus(SC_INIT_FAILED,GetLastError());
			iPort = ServntMgr::randInt(5000,6000);
			addr.sin_port=htons(iPort);
		}
		else
		{
			IniUtil::setLocalPort(iPort);
			break;
		}
	}
	//TCP listener
	if( bRunAsTCP &&  SOCKET_ERROR == listen(m_skt,10))
	{
#ifdef _DEBUG
		std::cout << "Can't Start Listen " <<  GetLastError();
#endif
		setStatus(SC_INIT_FAILED,GetLastError());
	}


	//主工作线程
	ThreadInfo *lpInfo = new ThreadInfo();
	lpInfo->func = (THREAD_FUNC)workThreadFunc;
	lpInfo->data = lpData;
	lpInfo->isWorkFuncThread = true;
	vtThreads.push_back(lpInfo);
	sys->startThread(lpInfo);


	//数据处理线程的辅助线程

	lpInfo = new ThreadInfo();
	lpInfo->func = (THREAD_FUNC)workHelperThreadFunc;
	lpInfo->data = lpData;
	vtThreads.push_back(lpInfo);
	sys->startThread(lpInfo);

	//Udp处理线程
	lpInfo = new ThreadInfo();
	lpInfo->func = (THREAD_FUNC)UdpWrkThread;
	lpInfo->data = lpData;
	vtThreads.push_back(lpInfo);
	sys->startThread(lpInfo);

	
	//把自己加入mCache中
	ServantInfo *info = new ServantInfo();
	info->dwSID = dwSID;
//	info.wSeqID = 0;
	info->bIsMySelf = true;
	//time(&info.tLastUpdate);
//	info.m_skt =m_skt;//
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = IniUtil::getLocalPort();
	localAddr.sin_addr.s_addr = IniUtil::getLocalIP();
	

	memset(&(localAddr.sin_zero), '\0', 8);
	
	memcpy(&info->addr,&localAddr,sizeof(localAddr));

	lpCacheLock->on();
	mCache.push_back(info);
	lpCacheLock->off();
	info->lpSktObj =  Append(m_skt,true);
	info->lpSktObj->lpmCacheNode = info;
	memcpy(&info->lpSktObj->addr,&localAddr,sizeof(localAddr));
	info->lpSktObj->addrlen = sizeof(localAddr);

	time(&info->lpSktObj->tLastUpdate);

	setStatus(SC_INIT_FINISHED);
}

//作为服务器启动
void ServntMgr::srv_Start(DWORD ChID)
{
	
	bIsRunning=true;
	bIsSrv = true;
	initMgr();

	//生成ServantID
	dwSID = localAddr.sin_addr.s_addr;
	(*mCache.begin())->dwSID = dwSID; //修正mCache中错误的本机SID
	//修正vtSocket中的数据
	SocketObj *lpSktObj = Find(0);
	lpSktObj->dwChID=ChID;
	lpSktObj->sid = dwSID;

#ifdef _DEBUG
	cout << "SID:0x" << dwSID << endl;
#endif // _DEBUG

	memcpy(&serverAddr,&localAddr,sizeof(localAddr)); // fix server address by exchange bm proc

	dwChID = ChID;

	
	lpSrcPin= new SourcePin();
	lpSrcPin->Start(lpBuf);
	
	this->startScheduler();
	
	
	while(bIsRunning)
	{
		Sleep(50);
	}
}

//作为客户端启动
void ServntMgr::Start(char *szIP, WORD wPort, DWORD ChID)
{
	//初始化 socketIO模型相关资源
	if (bIsRunning)
	{
		if (ChID == this->dwChID)
		{
			return;
		} 
		else
		{
			Stop();
		}
	}
	m_UdpIsOK=false;
	bIsRunning = true;
	bIsSrv = false;
	dwSID = 0;//must be zero
	dwChID = ChID;

#ifndef _WINDLL

	initMgr();

	
	setStatus(SC_CONNECT_STARTED);

	//投递一个加入请求

	MsgJoin *lpMsg = new MsgJoin(dwSID,dwChID);

	sockaddr_in raddr;
	raddr.sin_family = AF_INET;
	raddr.sin_port =  htons(wPort);
	raddr.sin_addr.s_addr = inet_addr(szIP);
	memset(&(raddr.sin_zero), '\0', 8);
	lpMsg->vtIP = vtLocalIps;
	lpMsg->wPort = IniUtil::getLocalPort();

	memcpy(&serverAddr,&raddr,sizeof(raddr));
	//	SendTo(lpMsg);

	SOCKET skt ;
	if(bRunAsTCP)
		skt = sys->createSocket(SOCK_STREAM);
	else
		skt = m_skt;


	SocketObj *lpSrvSkt = Append(skt); // lpSrvSkt's lpmCacheNode is NULL,need to fix

	lpSrvSkt->addrlen = sizeof(raddr);
	memcpy(&lpSrvSkt->addr ,&raddr,lpSrvSkt->addrlen);

	lpSrvSkt->SendTo(lpMsg);

	while(bIsRunning)
	{
		Sleep(50);
	}
#else
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(szIP);
	serverAddr.sin_port = htons(wPort);
	memset(&(serverAddr.sin_zero), '\0', 8);

	ThreadInfo *lpCltWrk = new ThreadInfo();
	lpCltWrk->data = this;
	lpCltWrk->func = (THREAD_FUNC)CltWrkThread;

	vtThreads.push_back(lpCltWrk);
	

	sys->startThread(lpCltWrk);
	Sleep(150);
#endif
	
}
int ServntMgr::CltWrkThread(ThreadInfo *lpInfo)
{
	ServntMgr *me = (ServntMgr*)lpInfo->data;

	me->initMgr();

	me->setStatus(SC_CONNECT_STARTED);

	//投递一个加入请求

	MsgJoin *lpMsg = new MsgJoin(me->dwSID,me->dwChID);


	lpMsg->vtIP = me->vtLocalIps;
	lpMsg->wPort = IniUtil::getLocalPort();

	
	//	SendTo(lpMsg);

	SOCKET skt ;
	if(me->bRunAsTCP)
		skt = sys->createSocket(SOCK_STREAM);
	else
		skt = me->m_skt;


	SocketObj *lpSrvSkt = me->Append(skt); // lpSrvSkt's lpmCacheNode is NULL,need to fix

	lpSrvSkt->addrlen = sizeof(me->serverAddr);
	memcpy(&lpSrvSkt->addr ,&me->serverAddr,lpSrvSkt->addrlen);

	lpSrvSkt->SendTo(lpMsg);


	while(me->bIsRunning)
	{
		Sleep(50);
	}
	return 0;
}

void ServntMgr::Stop(bool bIsRestart ,int ErrorCode)
{
	if(!bIsRunning)
	{
		return;
	}
	if (bIsSrv)
	{
		lpSrcPin->Stop();
	}

	this->stopScheduler(bIsRestart);
	if (!bIsSrv)
	{
		lpHttpDeliver->Stop();
		closesocket(m_UdpSkt);
	}
	else
	{
		lpSrcPin->Stop();
	}

	//发送Leave消息
	lpCacheLock->on();
	for(vector<ServantInfo*>::iterator it = mCache.begin();it!=mCache.end();it++)
	{
		if (NULL!=(*it)->lpSktObj && ! (*it)->bIsMySelf)
		{
			MsgLeave *lpLeave = new MsgLeave();
			(*it)->lpSktObj->SendTo(lpLeave);
		}
	}
	lpCacheLock->off();
	Sleep(100);

	bIsRunning=false;
	Sleep(70);
	
	//停止线程
	for(vector<ThreadInfo*>::iterator it = vtThreads.begin();it!=vtThreads.end();)
	{
		if ((*it)->isWorkFuncThread)
		{
			ThreadData *lpData = (ThreadData*)(*it)->data;
							
			while(!lpData->vtSocket.empty())
			{
				SocketObj* _SktObj = lpData->vtSocket.front();
				shutdown(_SktObj->skt,SD_BOTH);
				closesocket(_SktObj->skt);
				Remove(_SktObj);
			}
			if (lpData->lpQueue!=NULL)
			{
				delete lpData->lpQueue;
				lpData->lpQueue=NULL;
			}
			delete lpData;
		}
		delete *it;
		it = vtThreads.erase(it);
	}
	//删除所有连接关闭socket
	while (!mCache.empty())
	{

		if (mCache.front()->lpSktObj == NULL)
		{
			delete mCache.front();
			mCache.erase(mCache.begin());
		}
		else
		{
			/*shutdown(mCache.front()->lpSktObj->skt,SD_BOTH);
			Sleep(5);
			closesocket(mCache.front()->lpSktObj->skt);*/
			Remove(mCache.front()->lpSktObj);
		}

	}	
	while (lpSvt->Count!=0)
	{
		Remove(lpSvt->getServantAt(0)->lpSktObj);
	}
	shutdown(m_skt,SD_BOTH);
	Sleep(5);
	closesocket(m_skt);

	shutdown(m_UdpSkt,SD_BOTH);
	Sleep(5);
	closesocket(m_UdpSkt);
#if DEBUG
	assert(mCache.empty());
	assert(vtThreads.empty());
	assert(lpSvt->Count==0);
#endif
	setStatus(ErrorCode);
}

SocketObj* ServntMgr::Append(SOCKET skt)
{
	
	return Append(skt,false);
}

SocketObj* ServntMgr::Append(SOCKET skt,bool bIsListening)
{
	
	vector<ThreadInfo*>::iterator it;
	bool bAppend = false;
	SocketObj *lpSktObj = NULL;
	for(it = vtThreads.begin();it!=vtThreads.end();it++)
	{
		if ((*it)->isWorkFuncThread)
		{
		
			ThreadData *lpData = (ThreadData*)(*it)->data;
			bAppend = lpData->Append(skt,bIsListening,&lpSktObj);
			if(bAppend)
				break;
		}
	}
	if(!bAppend)
	{
		//增加新的线程
		ThreadData *lpData = new ThreadData(this);
		//主工作线程
		ThreadInfo *lpInfo = new ThreadInfo();
		lpInfo->func = (THREAD_FUNC)workThreadFunc;
		lpInfo->data = lpData;
		lpInfo->isWorkFuncThread = true;
		vtThreads.push_back(lpInfo);
		sys->startThread(lpInfo);


		//数据处理线程的辅助线程

		lpInfo = new ThreadInfo();
		lpInfo->func = (THREAD_FUNC)workHelperThreadFunc;
		lpInfo->data = lpData;
		vtThreads.push_back(lpInfo);
		sys->startThread(lpInfo);

		bAppend = lpData->Append(skt,bIsListening,&lpSktObj);
	}
	lpSktObj->dwChID = dwChID;
	return lpSktObj;
}


SocketObj* ServntMgr::Find(PEERID dwSID)
{
	SocketObj *lpRet = NULL;
	lpThreadsLock->on();
	for(vector<ThreadInfo *>::iterator it = vtThreads.begin(); it != vtThreads.end(); it++)
	{
		if((*it)->isWorkFuncThread)
		{
			ThreadData *lpData =(ThreadData *) (*it)->data;
			lpRet = lpData->Find(dwSID);
			if(NULL != lpRet)
				break;
		}
	}
	lpThreadsLock->off();
	return lpRet;
}

void ServntMgr::Remove(SocketObj *lpSktObj)
{
	//shutdown(lpSktObj->skt,SD_BOTH);
	//closesocket(lpSktObj->skt);

	//remove from mCache
	if( NULL != lpSktObj->lpmCacheNode )
	{
		lpCacheLock->on();	
		
		for(vector<ServantInfo*>::iterator it =mCache.begin(); it != mCache.end(); )
		{
			if((*it)->dwSID == lpSktObj->lpmCacheNode->dwSID)
			{
				
				delete lpSktObj->lpmCacheNode;
				lpSktObj->lpmCacheNode = NULL;
				it = mCache.erase(it);
				break;
			}
			else
			{
				it++;
			}
		}
		lpCacheLock->off();	
	}

	//remove from ServantList
	if(NULL != lpSktObj->lpServant)
	{
		lpSvt->Remove(lpSktObj->lpServant);
	}

	bool bFound = false;
	ThreadData *lpData = NULL;
	lpThreadsLock->on();
	for(vector<ThreadInfo *>::iterator it = vtThreads.begin(); it != vtThreads.end(); it++)
	{
		if((*it)->isWorkFuncThread)
		{
			lpData =(ThreadData *) (*it)->data;
			
			bFound = lpData->Remove(lpSktObj);
			
			if(bFound)
			{
				break;
			}
			
		}
	}
	

	//remove queued message it's received
	lpData->lpQueue->Remove(lpSktObj);

	//remove socketobject itself	

	delete lpSktObj;
	
	lpThreadsLock->off();
	
}

//工作者线程
int ServntMgr::workThreadFunc(ThreadInfo *lpInfo)
{
	ThreadData *lpData = (ThreadData *)lpInfo->data;
//	char buf[PACKET_SIZE];
	FD_SET set_rds,set_wrs,set_expt;
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100;

	bool bDisconnected = false;

	while(lpData->lpMgr->bIsRunning)
	{
		Sleep(1);
		FD_ZERO(&set_rds);
		FD_ZERO(&set_wrs);
		FD_ZERO(&set_expt);
		bDisconnected = false;

		lpData->m_Lock->on();
		if(lpData->lpMgr->bRunAsTCP)
		{
			for(vector<SocketObj*>::iterator it = lpData->vtSocket.begin() ; it != lpData->vtSocket.end(); it++)
			{
				SocketObj *lpSktObj = *it;
				if (!lpSktObj->bIsListening && !lpSktObj->bIsConnected)
				{
					connect(lpSktObj->skt,(sockaddr*)&lpSktObj->addr,lpSktObj->addrlen);
					//lpSktObj->bIsConnected = true;
					
				}
				FD_SET((*it)->skt,&set_rds);
				FD_SET((*it)->skt,&set_wrs);
				FD_SET((*it)->skt,&set_expt);
			}			
		}
		else
		{
			FD_SET(lpData->lpMgr->m_skt,&set_rds);
			FD_SET(lpData->lpMgr->m_skt,&set_wrs);
		}
		if ( 0 == set_rds.fd_count )
		{
			lpData->m_Lock->off();
			continue;
		}
		int rc ;
		if(lpData->bRunAsTCP)
			rc = select(0,&set_rds,&set_wrs,&set_expt,&tv);
		else
			rc = select(0,&set_rds,NULL,NULL,&tv);

		
		if(SOCKET_ERROR == rc)
		{
			DWORD dwErr = GetLastError();
			if (0 !=dwErr)
			{
#ifdef _DEBUG
				std::cout << "select  ERROR:" << dwErr << endl;
#endif
				lpData->lpMgr->setStatus(102,GetLastError());
			}
			lpData->m_Lock->off();
			continue;
		}

		vector<SocketInfo> vtSkt_t; //用于存储accept的socket对象
		//vector<SocketObj*> vtSktObjs ;//用于存储已经断开的连接
		map<PEERID,SocketObj*> mapSktObjsToDel;// 用于存储已经断开的连接

		
		for(vector<SocketObj*>::iterator it = lpData->vtSocket.begin() ; it != lpData->vtSocket.end(); it++)
		{
			SocketObj *lpSktObj = *it;
			//TCP 服务专用
			if(FD_ISSET(lpSktObj->skt,&set_rds)) //can read
			{
				if(lpSktObj->bIsListening)
				{
					SocketInfo info;
					info.addrlen = sizeof(info.addr);
					
					SOCKET skt =INVALID_SOCKET;//
					while (INVALID_SOCKET == skt && lpData->lpMgr->bIsRunning)
					{
						skt = accept(lpData->lpMgr->m_skt,(SOCKADDR*)&info.addr,&info.addrlen);
						if (WSAEWOULDBLOCK != GetLastError() && GetLastError()!= 0)
						{
#ifdef _DEBUG
							std::cout<< "Accept Error:" << GetLastError() << endl;
#endif
							lpData->lpMgr->setStatus(SC_INIT_FAILED,GetLastError());
							break;
						}
						
					}
					if (INVALID_SOCKET == skt)
					{
						continue;
					}
					info.skt= skt;
					vtSkt_t.push_back(info); //将skt存入临时向量中，为的是不使it失效
					
				}
				else
				{
					try
					{
						lpSktObj->Receive(lpData->lpQueue);
					}
					catch (SocketException* )
					{
						//vtSktObjs.push_back(lpSktObj);
						if(mapSktObjsToDel.find(lpSktObj->sid)==mapSktObjsToDel.end())
							mapSktObjsToDel[lpSktObj->sid]=lpSktObj;
					}
					
				}//end of TCP专用
			} //end of rds
			
			if(!lpSktObj->bIsListening && FD_ISSET(lpSktObj->skt,&set_wrs)) //can write
			{
				lpSktObj->bIsConnected = true;
				lpSktObj->bCanWrite = true;
				try
				{
					lpSktObj->SendTo_s();
				}
				catch(SocketException*)
				{
					if(mapSktObjsToDel.find(lpSktObj->sid)==mapSktObjsToDel.end())
						mapSktObjsToDel[lpSktObj->sid]=lpSktObj;
				}
			}
			else if(0 == rc && !lpSktObj->bIsListening)
			{
				try
				{
					lpSktObj->SendTo_s();
				}
				catch(SocketException*)
				{
					if(mapSktObjsToDel.find(lpSktObj->sid)==mapSktObjsToDel.end())
						mapSktObjsToDel[lpSktObj->sid]=lpSktObj;
				}
			}
			if(FD_ISSET(lpSktObj->skt,&set_expt))
			{
				if(mapSktObjsToDel.find(lpSktObj->sid)==mapSktObjsToDel.end())
				{
					mapSktObjsToDel[lpSktObj->sid]=lpSktObj;
					bDisconnected = true;
				}
			}
		}//	
		lpData->m_Lock->off();
		
		//accept new connection
		for(vector<SocketInfo>::iterator it = vtSkt_t.begin() ; it != vtSkt_t.end(); it++)
		{
			
			SocketObj* lpSktObj = lpData->lpMgr->Append((*it).skt);
			lpSktObj->bIsConnected = true;	
			memcpy(&lpSktObj->addr,&(*it).addr,(*it).addrlen);
			lpSktObj->addrlen = (*it).addrlen;
			lpSktObj->lpmCacheNode = NULL;
			/*ServantInfo* info = new ServantInfo();
			info->lpSktObj = lpSktObj;
			memcpy(&info->addr,&lpSktObj->addr,(*it).addrlen);
			info->bIsMySelf = false;

			lpSktObj->lpmCacheNode = info;
			lpData->lpMgr->lpCacheLock->on();
			lpData->lpMgr->mCache.push_back(info);
			lpData->lpMgr->lpCacheLock->off();*/
		}

		//delete servant
		/*for (vector<SocketObj*>::iterator it = vtSktObjs.begin();it!=vtSktObjs.end();it++)
		{
			lpData->lpMgr->Remove(*it);
		}*/
		for (map<PEERID,SocketObj*>::iterator it = mapSktObjsToDel.begin();it!=mapSktObjsToDel.end();it++)
		{
			
			lpData->lpMgr->Remove(it->second);
			if (lpData->lpMgr->mCache.size() == 1 && bDisconnected)
			{
				//无法连接服务器
				//lpData->lpMgr->setStatus(SC_CONNECT_FAILED);
				lpData->lpMgr->Stop(false,14);
				return -1;
			}
		}

		
	}

	return -1;
}


/*************************************************************************************************************
 *
 *	数据处理辅助线程
 *
 *
 *************************************************************************************************************/


int ServntMgr::workHelperThreadFunc(ThreadInfo *lpInfo)
{
	ThreadData *lpData = (ThreadData *)lpInfo->data;
	char *buf = NULL;//new char[PACKET_SIZE];
	int ret;
	SocketObj *lpSktObj;
	while(lpData->lpMgr->bIsRunning)
	{
		
		if (lpData->lpQueue == NULL)
		{
			Sleep(1);
			continue;
		}
		lpData->lpQueue->Pop(&buf,&ret,&lpSktObj);
		if(ret == -1)
		{
			Sleep(1);
			continue;
		}
		WORD wCmd ,wSeq;

		if( 0 == ret)
		{
			wCmd = CMD_LEAVE;//接收到0字节，等于离开
		}
		else
		{
			memcpy(&wCmd,buf,2);
			memcpy(&wSeq,buf+2,2);
		}
	#if _DEBUG
			cout<< " Queue Size:" << lpData->lpQueue->Size() << endl;
	#endif

		//如果拿到确认消息，从消息队列中移除
		
		switch(wCmd)
		{
		case CMD_UNKNOWN:
			{
				shutdown(lpSktObj->skt,SD_BOTH);
				Sleep(20);
				closesocket(lpSktObj->skt);
				lpData->lpMgr->Remove(lpSktObj);
			}
			break;
		case CMD_JOIN: //收到加入请求
			{
				//generate low id
				static DWORD dwLowID = MAX_LOWID;

				MsgJoin *lpCnn = new MsgJoin(buf,ret);
				
				lpSktObj->sid = lpCnn->dwSID;
				//Join Resp
				MsgJoinR *lpCnnR = new MsgJoinR();
				lpCnnR->wSeqID = lpCnn->wSeqID;
				lpCnnR->dwServerSID = lpData->lpMgr->dwSID;
				
				
				//检查是否超过本节点能够容纳的最大Servant数量
				if (lpData->lpMgr->lpSvt->Count > MAX_SERVANT_COUNT)
				{
					lpCnnR->Result = Join_Result_ReDirection;
				}
				else
				{
					if (lpData->lpMgr->dwChID!=lpCnn->dwChID)
						lpCnnR->Result=Join_Result_ChID_Not_Match;
					else
						lpCnnR->Result = Join_Result_OK;
				}

				if ( 0  == lpCnn->dwSID)
				{
					for (vector<DWORD>::iterator it = lpCnn->vtIP.begin() ; it != lpCnn->vtIP.end() ; it++)
					{

						if (lpSktObj->addr.sin_addr.s_addr == *it) // 客户提供的IP与检测到的IP相同，说明是公网节点
						{
							// high id is the ip address
							lpCnnR->dwClientSID = lpSktObj->addr.sin_addr.s_addr;
							//回连检测。通过本端再回连到客户端进一步确定是否是公网节点
							SOCKET skt_t = sys->createSocket(SOCK_STREAM);
							sys->setNoBlock(skt_t);
							sockaddr_in addr_t;
							addr_t.sin_family = AF_INET;
							addr_t.sin_addr.s_addr = lpSktObj->addr.sin_addr.s_addr;
							addr_t.sin_port = lpCnn->wPort;
							memset(addr_t.sin_zero,0,sizeof(addr_t.sin_zero));
							DWORD dwStartTick = GetTickCount();
							int ret = -1;
							while(1)
							{
								ret  = connect(skt_t,(sockaddr*)&addr_t,sizeof(addr_t));
								switch(GetLastError())
								{
								case WSAEWOULDBLOCK:
									break;
								case WSAEISCONN: //already connect
									ret = 0;
								    break;
								case 0:
									break;
								default:
								    break;
								}
								if (GetTickCount() - dwStartTick >= 300 || ret == 0)
								{
									break;
								}
								Sleep(5);
							}
							
							if (ret != 0) //连接失败，说明还是lowid
							{
								lpCnnR->dwClientSID = --dwLowID;
							}
							else
							{	
								//发送unknown
								char buf_t[6] ={(char)0x00,(char)0x00,(char)0x00,(char)0x00,(char)0x06,(char)0x00};
								//buf_t[0] = 
								ret = send(skt_t,buf_t,sizeof(buf_t),0);
							}
							static   struct   linger   lig;   
							lig.l_onoff=1;   
							lig.l_linger=0;   
							static   int   iLen=sizeof(struct   linger);   
							setsockopt(skt_t,SOL_SOCKET,SO_LINGER,(char   *)&lig,iLen);   
							
							ret = shutdown(skt_t,SD_BOTH);
							Sleep(2);//
							ret = closesocket(skt_t);
							break;
						}
					}
					
					if (0 == lpCnnR->dwClientSID)
					{
						
						lpCnnR->dwClientSID = --dwLowID;
					}
				}
				else
				{
					lpCnnR->dwClientSID = lpCnn->dwSID;
				}
				
				lpCnn->dwSID = lpCnnR->dwClientSID;
				lpSktObj->sid = lpCnn->dwSID;

				lpSktObj->SendTo(lpCnnR);//Join Resp
				if (lpCnnR->Result==Join_Result_ChID_Not_Match)
				{
					break;
				}
				//投递一个PushServans消息 
				//对于每个最大的PACK_DATA_SIZE的包，一次可以传送大约682个servant数据。
				MsgPushServant *lpMsgSvt = new MsgPushServant();

				lpMsgSvt->vtSvts = lpData->lpMgr->mCache;

				lpSktObj->SendTo(lpMsgSvt);//Push Servants

				if( NULL == lpSktObj->lpmCacheNode && static_cast<int>(lpData->lpMgr->mCache.size()) < MAX_MCACHE_COUNT )
				{
					ServantInfo* svtInfo = new ServantInfo();
					//memcpy(&svtInfo.addr ,&lpSktObj->addr,lpSktObj->addrlen);

					svtInfo->addr.sin_addr.s_addr = lpSktObj->addr.sin_addr.s_addr;
					svtInfo->addr.sin_port = lpCnn->wPort;

					svtInfo->dwSID = lpCnn->dwSID;
					svtInfo->lpSktObj = lpSktObj;

					lpSktObj->lpmCacheNode = svtInfo;
					time(&svtInfo->lpSktObj->tLastUpdate);
					//						svtInfo.wSeqID = wSeq;
					//加入到mCache中
					lpData->lpMgr->lpCacheLock->on();

					lpData->lpMgr->mCache.push_back(svtInfo);
					lpData->lpMgr->lpCacheLock->off();
				}

				//投递一个PushHeader消息
				MsgPushHeader *lppHdr = new MsgPushHeader();

				lppHdr->wInitSegID=lpData->lpMgr->lpBuf->getStartID();
				lppHdr->dataLen = lpData->lpMgr->lpBuf->m_Header.len;
				lppHdr->lpData = lpData->lpMgr->lpBuf->m_Header.lpData;
				lpSktObj->SendTo(lppHdr);//Push Header .control data
								
				

				delete lpCnn;
			}
			break;
		case CMD_JOIN_R://加入响应
			{
				MsgJoinR *lpCnnR = new MsgJoinR(buf,ret);

				lpData->lpMgr->SrvSID = lpCnnR->dwServerSID;
				lpData->lpMgr->dwSID = lpCnnR->dwClientSID;
				lpData->lpMgr->lpCacheLock->on();
				(*lpData->lpMgr->mCache.begin())->dwSID = lpCnnR->dwClientSID;//修复mCache中错误的本机SID
				lpData->lpMgr->lpCacheLock->off();
				lpSktObj->sid = lpCnnR->dwServerSID;//修复服务器的SID
#ifdef _DEBUG
				cout << "SID:" << lpData->lpMgr->dwSID << (IsHighID(lpData->lpMgr->dwSID)?"Is Hight ID":"Is Low ID") <<endl;
#endif
				
				switch(lpCnnR->Result)
				{
				case Join_Result_OK:
					{
						lpData->lpMgr->setStatus(SC_CONNECT_OK);

						//启动调试线程
						lpData->lpMgr->startScheduler();
						//投递一个Connect消息
						MsgConnect *lpCnt = new MsgConnect();
						lpCnt->dwSID = lpData->lpMgr->dwSID;


						lpSktObj->SendTo(lpCnt);
					}
					break;
				case Join_Result_ReDirection:
					{
						lpData->lpMgr->setStatus(SC_CONNECT_FAILED,Join_Result_ReDirection);
						//断开连接
						shutdown(lpSktObj->skt,SD_BOTH);
						Sleep(20);
						closesocket(lpSktObj->skt);
						lpData->Remove(lpSktObj);

						//启动调试线程。这里虽然没有和服务器建立连接，但已经拿到了servant，有足够的资料进行正常操作。
						lpData->lpMgr->startScheduler();

					}
					break;
				case Join_Result_Require_New_Version:
					lpData->lpMgr->setStatus(SC_CONNECT_FAILED,Join_Result_Require_New_Version);
					break;
				case Join_Result_ChID_Not_Match:
					lpData->lpMgr->setStatus(SC_CONNECT_FAILED,Join_Result_ChID_Not_Match);
					break;
				}
				delete lpCnnR;
			}
			break;
		case CMD_CONNECT :
			{
				MsgConnect *lpMsg = new MsgConnect();
				lpMsg->fromBytes(buf,ret);
				
				//反馈
				MsgConnectR *lpMsgR = new MsgConnectR();
				lpMsgR->wSeqID = lpMsg->wSeqID;
				lpMsgR->dwSID = lpData->lpMgr->dwSID;
				if (lpMsg->dwChID != lpData->lpMgr->dwChID)
				{
					lpMsgR->cRevert = 1;	//失败
				}
				else
				{
					if(lpData->lpMgr->lpSvt->Count>MAX_SERVANT_COUNT)//:检查是否可以接受连接。
						lpMsgR->cRevert=2;
					else
						lpMsgR->cRevert = 0;	//连接成功
				}
					

				lpMsgR->lpBM = lpData->lpMgr->lpBuf->m_lpBM->toBytes();

				lpData->lpMgr->lpCacheLock->on();
				for (vector<ServantInfo*>::iterator it = lpData->lpMgr->mCache.begin();it!=lpData->lpMgr->mCache.end();it++)
				{
					if ((*it)->dwSID == lpMsg->dwSID) //found
					{
						if (NULL == (*it)->lpSktObj)
						{
							(*it)->lpSktObj = lpSktObj;
							lpSktObj->lpmCacheNode = *it;
							
						} 
						break;
					}
				}
				lpData->lpMgr->lpCacheLock->off();
				

				if( NULL == lpSktObj->lpmCacheNode && static_cast<int>(lpData->lpMgr->mCache.size()) < MAX_MCACHE_COUNT )
				{
					ServantInfo* svtInfo = new ServantInfo();
					//memcpy(&svtInfo.addr ,&lpSktObj->addr,lpSktObj->addrlen);

					svtInfo->addr.sin_addr.s_addr = lpSktObj->addr.sin_addr.s_addr;
					svtInfo->addr.sin_port = lpSktObj->addr.sin_port;

					svtInfo->dwSID = lpMsg->dwSID;
					svtInfo->lpSktObj = lpSktObj;

					lpSktObj->lpmCacheNode = svtInfo;
					time(&svtInfo->lpSktObj->tLastUpdate);
					//						svtInfo.wSeqID = wSeq;
					//加入到mCache中
					lpData->lpMgr->lpCacheLock->on();

					lpData->lpMgr->mCache.push_back(svtInfo);
					lpData->lpMgr->lpCacheLock->off();
				}
				//lpSktObj->lpmCacheNode->dwSID= lpMsg->dwSID;

				lpSktObj->SendTo(lpMsgR);

				Servant *lpSv_t = new Servant(lpMsg->dwSID,lpSktObj); // create new Servant instance and assign lpSktObj to the servant		

				//Append to Servant List

				lpData->lpMgr->lpSvt->Append(lpSv_t);
				
				delete lpMsg;
			}
			break;
		case CMD_CONNECT_R :
			{
				MsgConnectR *lpMsg = new MsgConnectR();
				lpMsg->fromBytes(buf,ret);

				if(0 != lpMsg->cRevert)	//拒绝连接
				{
					shutdown(lpSktObj->skt,SD_BOTH);
					Sleep(20);
					closesocket(lpSktObj->skt);
					lpData->lpMgr->Remove(lpSktObj);
				}
				else
				{

					Servant *lpSv_t = new Servant(lpMsg->dwSID,lpSktObj);

					lpSv_t->getBufferMap()->fromBytes(lpMsg->lpBM);
					lpData->lpMgr->lpBuf->wServerCurrSegID = lpSv_t->getBufferMap()->getStartID();
					//lpData->lpMgr->Append(lpSv_t);
					lpSktObj->lpServant = lpSv_t;
					if( NULL == lpSktObj->lpmCacheNode )
					{
						ServantInfo *svtInfo = new ServantInfo();
						memcpy(&svtInfo->addr ,&lpSktObj->addr,lpSktObj->addrlen);
						svtInfo->dwSID = lpSv_t->getSID();
						svtInfo->lpSktObj = lpSktObj;
						time(&svtInfo->lpSktObj->tLastUpdate);
						lpSktObj->lpmCacheNode = svtInfo;
						//						svtInfo.wSeqID = wSeq;
						//加入到mCache中
						lpData->lpMgr->lpCacheLock->on();

						lpData->lpMgr->mCache.push_back(svtInfo);
						lpData->lpMgr->lpCacheLock->off();
						
					}
					lpData->lpMgr->lpSvt->Append(lpSv_t);

				}

				delete lpMsg;
			}
			break;
		case CMD_PUSH_HEADER://推送头
			{
				MsgPushHeader *lpMsg = new MsgPushHeader();
				lpMsg->fromBytes(buf,ret);		


				////反馈Push Header Response
				//MsgPushHeaderR *lpMsgR = new MsgPushHeaderR(wSeq);
				//lpSktObj->SendTo(lpMsgR);
				lpData->lpMgr->lpBuf->m_Header.dwStartID = lpMsg->wInitSegID;
				if( NULL == lpData->lpMgr->lpBuf->m_Header.lpData)
				{
					lpData->lpMgr->lpBuf->m_Header.lpData = new char[lpMsg->dataLen];
				}
				lpData->lpMgr->lpBuf->m_Header.len =lpMsg->dataLen;
				memcpy(lpData->lpMgr->lpBuf->m_Header.lpData,lpMsg->lpData,lpMsg->dataLen);
				
				//分析头，初始化Segment::Packet_Of_Segments、Last_Packet_Size等
				asfHeader *objHeader = new asfHeader(lpData->lpMgr->lpBuf->m_Header.lpData,lpData->lpMgr->lpBuf->m_Header.len);
				DWORD dwPacketSize = objHeader->getChunkSize();
				DWORD dwBitRate = objHeader->getBitRate();
				delete objHeader;

				int size = ( dwBitRate /8 /dwPacketSize)* dwPacketSize;//(349056 /8 /0x5a4) * 0x5a4;
				Segment::setSegmentSize(size);
				Segment::setChunkSize(dwPacketSize);

				lpData->lpMgr->lpBuf->Init(lpData->lpMgr->lpBuf->m_Header.dwStartID); //初始化Buffers
				lpData->lpMgr->setStatus(SC_CMD_GETHEAD);
	
				delete lpMsg;
				
			}
			break;
		case CMD_PUSH_HEADER_R://推送头响应
			{
			}
			break;

		case CMD_PUSH_SERVANTS://推送servants 
			{
				MsgPushServant *lpMsg = new MsgPushServant(buf,ret);

				/*MsgPushServantR *lpMsgR = new MsgPushServantR(wSeq);

				lpSktObj->SendTo(lpMsgR);*/

				lpData->lpMgr->lpCacheLock->on();
				for(vector<ServantInfo*>::iterator it = lpMsg->vtSvts.begin(); it != lpMsg->vtSvts.end(); it++)
				{
					if((*it)->dwSID == lpData->lpMgr->dwSID || (*it)->dwSID ==0 ) //不处理自身
					{
						delete (*it);
						continue;
					}
					ServantInfo* lpInfo = *it;
					//memcpy(lpInfo,&(*it),sizeof(ServantInfo));

					lpInfo->lpSktObj = lpData->lpMgr->Find(lpInfo->dwSID);
					/*if(lpSktObj->sid == lpInfo->dwSID)
					{
						
						lpInfo->lpSktObj = lpSktObj;
					}*/
					if(NULL == lpInfo->lpSktObj)
					{
						
						//如果本端是LowID且传过来的仍然是LowID，则不保存
						if (!(IsLowID(lpData->lpMgr->dwSID) && IsLowID(lpInfo->dwSID)))
						{
							lpData->lpMgr->mCache.push_back(lpInfo); //just save the servant info
						}

					}
					else
					{
						if (NULL == lpSktObj->lpmCacheNode)
						{
							lpInfo->lpSktObj->addrlen = sizeof(lpInfo->addr);
							memcpy(&lpInfo->lpSktObj->addr,&lpInfo->addr,sizeof(lpInfo->addr));

							//(*it).lpSktObj->bIsConnected = false;
							lpInfo->lpSktObj->lpmCacheNode = lpInfo;
							lpData->lpMgr->mCache.push_back(lpInfo);
						}
						
					}

					
				}
				lpData->lpMgr->lpCacheLock->off();
				
				

				delete lpMsg;

			}
			break;
		case CMD_PUSH_SERVANTS_R://推送Servants响应	。nothing to do
			break;
	
		case CMD_LEAVE://离开
			{
				//从自己的ServantList中删除这个节点
				lpData->lpMgr->Remove(lpSktObj);	
			}
			break;
		case CMD_LEAVE_R://离开响应	'Ex1中离开消息不需要响应
			break;
		case CMD_EXCHANGE_BM://激活
			{
				MsgExBM *lpMsg = new MsgExBM();

				lpMsg->fromBytes(buf,ret);
				Servant *lpSv = lpSktObj->lpServant;
				
				if(NULL == lpSv)
					break;
				
				/*lpSv->setBandWidth(lpMsg->wBandWidth);
				lpSv->setNumberOfPartners(lpMsg->wNumberOfPartners);
				lpSv->setRemoteBytesOfReceive(lpMsg->dwRemoteBytesOfReceive);
				lpSv->setRemoteBytesOfSend(lpMsg->dwRemoteBytesOfSent);*/
				lpSv->m_BM->fromBytes(lpMsg->lpBM);
				if(lpSktObj->wSndSegID< lpSv->m_BM->getStartID())
					lpSktObj->wSndSegID = lpSv->m_BM->getStartID();

	//									assert(lpSv->m_BM->m_Bits.count()==60);
				//如果本端的StartID与ExchangeMap端的StartID相差超过SyncID要求的时间，则说明UDP的SyncID失败，马上执行SyncID需要处理的程序
				if (!lpData->lpMgr->m_UdpIsOK &&  lpSv->m_BM->getStartID() > lpData->lpMgr->lpBuf->getStartID() +SYNC_ID_TIMER / 1000  )
				{
					DoSyncIDProcess(lpSv->m_BM->getStartID(),lpData->lpMgr);
				}
				delete lpMsg;
			}
			break;
//		case CMD_EXCHANGE_BM_R://激活响应	'激活消息无需响应
//			break;
		case CMD_APPLY_DATAS://请求数据
			{
				MsgApplyData *lpMsg = new MsgApplyData(lpData->lpMgr->dwSID);
				lpMsg->fromBytes(buf,ret);

				Servant* lpSv = lpSktObj->lpServant;
				if(NULL == lpSv)
					break;
				
				//计算可以提供的数据
				
				lpData->lpMgr->lpBuf->m_lpBM->checkData(lpMsg->vtSegIDs);
				MsgApplyDataR *lpMsgR = new MsgApplyDataR(lpMsg->dwSID);
				lpMsgR->wSeqID = lpMsg->wSeqID;
				
				lpMsgR->vtSegIDs = lpMsg->vtSegIDs;
		
				lpSktObj->SendTo(lpMsgR);		//ApplyData Response

				//推送数据	
				for(vector<SEGMENTID>::iterator it = lpMsg->vtSegIDs.begin();it!=lpMsg->vtSegIDs.end();it++)
				{
					Segment *lpSegment = lpData->lpMgr->lpBuf->getSegment(*it);
					if ( NULL == lpSegment )
						continue;
					//lpSegment->m_Lock->on();
					//分数据推送
					
					lpData->lpMgr->lpSvt->m_Lock->on();

					char* lpSegData = NULL; 
					if(! lpSegment->getData(&lpSegData))
					{
						lpData->lpMgr->lpSvt->m_Lock->off();
						//lpSegData==NULL
						continue;
					}
					lpData->lpMgr->lpSvt->m_Lock->off();
					int j;

					for(j = 0;j < Segment::getPacketsOfSegment();j++)
					{
						MsgPushData *lpMsg = new MsgPushData();

						
						if(j == Segment::getPacketsOfSegment()-1) //最后一个包
						{
							lpMsg->dataLen = Segment::getLastPacketSize();
						}
						else
						{
							lpMsg->dataLen = Segment::getRealPacketSize();
						}

						lpMsg->lpData = new char[lpMsg->dataLen];
						lpMsg->dwSegID = *it;
						lpMsg->cSeq = static_cast<char>(j);
						
						
						memcpy(lpMsg->lpData,lpSegData+(j* Segment::getRealPacketSize()),lpMsg->dataLen );

						lpSktObj->SendTo(lpMsg);
					}
					assert(j==Segment::getPacketsOfSegment());
					delete[] lpSegData;
				}
				delete lpMsg;

			}
			break;
		case CMD_APPLY_DATAS_R://请求数据响应
			{
				MsgApplyDataR *lpMsg = new MsgApplyDataR(lpData->lpMgr->dwSID);
				//将确认的数据保存下来
				Servant* lpSv = lpSktObj->lpServant;
				if(lpSv != NULL)
					lpSv->ApplyDatas(lpMsg->vtSegIDs);

				delete lpMsg;
			}
			break;
		case CMD_APPLY_PACKET: //请求特定的数据包
			{
				MsgApplyPacket *lpMsg = new MsgApplyPacket(buf,ret);
				Segment* lpSeg = lpData->lpMgr->lpBuf->getSegment(lpMsg->dwSegID);
				if(lpSeg != NULL)
				{
					char *lpSegData = NULL;
					lpSeg->getData(&lpSegData);

					MsgPushData *lpPushMsg = new MsgPushData();


					int j = lpMsg->cSeq;

					if(j == Segment::getPacketsOfSegment()-1) //最后一个包
					{
						lpPushMsg->dataLen = Segment::getLastPacketSize();
					}
					else
					{
						lpPushMsg->dataLen = Segment::getRealPacketSize();
					}

					lpPushMsg->lpData = new char[lpPushMsg->dataLen];
					lpPushMsg->dwSegID = lpMsg->dwSegID;
					lpPushMsg->cSeq = static_cast<char>(j);
																
					memcpy(lpPushMsg->lpData,lpSegData+(j*Segment::getRealPacketSize()),lpPushMsg->dataLen );

					delete[] lpSegData;

					lpSktObj->SendTo(lpPushMsg);

				}
				delete lpMsg;
			}
			break;
		case CMD_PUSH_DATA://推送数据
			{
				MsgPushData *lpMsg = new MsgPushData(buf,ret);
				PushDataResult rst = lpData->lpMgr->lpBuf->PushData(lpMsg->dwSegID,lpMsg->cSeq,lpMsg->
					lpData,lpMsg->dataLen);
				delete lpMsg;
				if (rst == PushData_OK)
				{
	
				lpData->lpMgr->setStatus(SC_BUFFER,lpData->lpMgr->lpBuf->m_lpBM->getFullRate() | (lpData->lpMgr->lpHttpDeliver->getBufferRate() >2) );
#if  _DEBUG
				cout << "Buffer Rate:" << lpData->lpMgr->lpBuf->m_lpBM->getFullRate() << "%" 
					 << "  Deliver Rate:" << lpData->lpMgr->lpHttpDeliver->getBufferRate() << "%"
					 << endl;
#endif
				}
				//如果Get_PUSH_DATA后缓冲已满，则调用SetPosition
				if(rst==PushData_OK && lpData->lpMgr->lpBuf->m_lpBM->getFullRate() > 96)
				{
					SEGMENTID cStartID = lpData->lpMgr->lpBuf->getStartID();
					lpData->lpMgr->lpBuf->wServerCurrSegID = cStartID + 5;
					lpData->lpMgr->lpSvt->processSevants_s(ServntMgr::SyncIDHelper,cStartID);
					lpData->lpMgr->lpBuf->SetPosition(cStartID); //调整StartID
					lpData->lpMgr->lpBuf->setTimer();
					lpSktObj->lpServant->SyncID(cStartID);	//adjust the applied data
				}
			}
			break;
		case CMD_PUSH_DATA_R://推送数据响应	nothing to do 
			break;
		case CMD_REJECT_DATA://拒绝数据	not implement in experiment1
			break;
		case CMD_REJECT_DATA_R://拒绝数据响应not implement in experiment1
			break;
		case CMD_CHANGE_MEDIA://媒体改变not implement in experiment1
			break;
		case CMD_CHANGE_MEDIA_R://媒体改变响应not implement in experiment1
			break;
		case CMD_PING:	//Ping指令 not need response
			{
				MsgPing *lpMsg = new MsgPing();
				lpMsg->fromBytes(buf,ret);
				assert(lpMsg->dwSID!=0);
				lpMsg->ttl --;
				lpMsg ->hops ++;
				bool bFind = false;

				lpData->lpMgr->lpCacheLock->on();
				for(vector<ServantInfo*>::iterator it = lpData->lpMgr->mCache.begin();it!= lpData->lpMgr->mCache.end();it++)
				{
					if((*it)->dwSID == lpMsg->senderSID )
					{
						bFind = true;
						break;
					}
				}
				lpData->lpMgr->lpCacheLock->off();

				//本端是LowID，且原始ping发起者也是LowID，则不保存
				if(!bFind && !(IsLowID(lpData->lpMgr->dwSID) && IsLowID(lpMsg->senderSID)) ) //将其加入到本机的mCache中
				{

					ServantInfo *info =new ServantInfo();
					info->addr.sin_family = AF_INET;
					info->addr.sin_addr.s_addr = lpMsg->dwIP;
					info->addr.sin_port = lpMsg->wPort;
					memset(info->addr.sin_zero,'\0',sizeof(info->addr.sin_zero));

					info->dwSID = lpMsg->senderSID;

					if (lpMsg->senderSID == lpMsg->dwSID) 
					{
						if (lpSktObj->lpmCacheNode == NULL)
						{
							lpSktObj->lpmCacheNode = info;
							info->lpSktObj = lpSktObj;

							lpData->lpMgr->lpCacheLock->on();

							lpData->lpMgr->mCache.push_back(info);

							lpData->lpMgr->lpCacheLock->off();
						}
						else
						{
							delete info;
						}
					}
					else
					{
	
						lpData->lpMgr->lpCacheLock->on();
						
						lpData->lpMgr->mCache.push_back(info);
						
						lpData->lpMgr->lpCacheLock->off();

					}
				}
				

				if(lpMsg->ttl <= 0) //不发送给其他客户端。直接丢弃
				{
				}
				else	//将ping转发给所以membership成员
				{
					lpData->lpMgr->lpCacheLock->on();

					for(vector<ServantInfo*>::iterator it = lpData->lpMgr->mCache.begin();it!= lpData->lpMgr->mCache.end();it++)
					{
						//防止发送给发起方，和上一个消息接收方以及自身。也不反馈给没有建立连接的servant
						if((*it)->dwSID != lpMsg->dwSID && (*it)->dwSID != lpData->lpMgr->dwSID && (*it)->lpSktObj !=NULL) 
						{
							MsgPing *lpMsg_t = new MsgPing();
							lpMsg_t->fromBytes(buf,ret);
							lpMsg_t->ttl --;
							lpMsg_t->hops ++;
							lpMsg_t->dwSID = lpData->lpMgr->dwSID;

							lpSktObj->SendTo(lpMsg_t);
						}
					}	

					lpData->lpMgr->lpCacheLock->off();
				}

				delete lpMsg ;
			}
			break;
		
		case CMD_SYNC_ID:	//同步ID
			{
				MsgSyncID *lpMsg = new MsgSyncID();
				lpMsg->fromBytes(buf,ret);
				MsgSyncIDR *lpMsgR = new MsgSyncIDR();
				lpMsgR->wSeqID = lpMsg->wSeqID;
				lpMsgR->dwSegID = lpData->lpMgr->lpBuf->getStartID();
				//删除在发送队列中SegmentID<当前SegmentID的数据，因为这些数据已经过期
				lpSktObj->SyncID(lpMsgR->dwSegID);
				lpSktObj->SendTo(lpMsgR);

				delete lpMsg;
			}
			break;
		case CMD_SYNC_ID_R: //同步ID响应
			{
				MsgSyncIDR *lpMsg = new MsgSyncIDR();
				lpMsg->fromBytes(buf,ret);
				SEGMENTID cStartID = lpData->lpMgr->lpBuf->getStartID();
				lpData->lpMgr->lpBuf->wServerCurrSegID = lpMsg->dwSegID;
				lpData->lpMgr->lpSvt->processSevants_s(ServntMgr::SyncIDHelper,cStartID);
				lpData->lpMgr->lpBuf->SetPosition(lpMsg->dwSegID); //调整StartID
				lpData->lpMgr->lpBuf->setTimer();
				lpSktObj->lpServant->SyncID(lpMsg->dwSegID);	//adjust the applied data
		
				delete lpMsg;
			}
			break;
		default:
			std::cout << "unknown command" << endl;
		} //switch

		delete[] buf; //destroy unused memory
	}
	return -2;
}
bool ServntMgr::SyncIDHelper(Servant* lpSvt,DWORD_PTR dwUser)
{
	lpSvt->lpSktObj->SyncID((WORD)dwUser);
	return true;
}
/*************************************************************************************************************
 *
 *		算法：计算每个SegmentID的候选提供者，然后根据候选提供者的优先级确定最终的提供者
 *		需要确定本调度程序的运行周期，还要确定已经发出applyData指令的segment不会出现过多的提供者，也不会向同一个servent多次发送同一segment的多次请求
 *
 *************************************************************************************************************/

void ServntMgr::schedulerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	ServntMgr *lpMgr = (ServntMgr*)dwUser;
	if(lpMgr->lpSvt->Count == 0)	//没有Servant连接，则退出
		return;
	//确定需要的Segment
	
	vector<SEGMENTID> vtExpect;
	
	lpMgr->lpBuf->m_lpBM->m_Lock->on();
	
	SEGMENTID wStartID = lpMgr->lpBuf->getStartID();
	for(int i = 0 ;i< static_cast<int>(lpMgr->lpBuf->m_lpBM->m_Bits.size()) ;i++)
	{
		if( ! lpMgr->lpBuf->m_lpBM->m_Bits.test(static_cast<size_t>(i)) )
		{
			vtExpect.push_back(wStartID + i);
		}
		
	}
	
	lpMgr->lpBuf->m_lpBM->m_Lock->off();

	//-----------------------------------------------------------

	vector<Servant *> vtSupplier;

	//处理所有预期的数据
	
	for(vector<SEGMENTID>::iterator itExp = vtExpect.begin();itExp != vtExpect.end();itExp++)
	{
		vtSupplier.clear();
		Segment* lpSeg = lpMgr->lpBuf->getSegment((*itExp));
		if(lpSeg == NULL)
			continue;

		vector<DWORD> &vtSpprSID = lpSeg->vtSuppliers;
		
		if ( NULL == lpSeg->t_ApplyTime)  /* never applied data 
			||	 difftime(time((time_t*)NULL) , lpSeg->t_ApplyTime)  > MAX_SEGMENT_COUNT / 5 
				&& lpSeg->getDeadline() >3 /* has applied ,but didn't get data,when this happened every MAX_SEGMENT /5 second can reapply 
			)*/
		{
			lpSeg->t_ApplyTime = NULL;
			vtSpprSID.clear();
		}
		lpMgr->lpSvt->m_Lock->on();
		
		ServantList::ServantNode *p =lpMgr->lpSvt->m_Root;//
		
		//确定每个Segment所有的提供者	
		while(NULL != p)
		{

			//需要数据并且没有向此节点请求过预期的数据。但是之前申请过数据的servant还会被加入。
			//因为没有检查已经请求过的数据，在这里是不需要检查这个的，为的是申请部分数据。这个检查会在后边做
			bool bNotRepeat = find(vtSpprSID.begin(),vtSpprSID.end(),p->Servant->getSID())==vtSpprSID.end();
			if( bNotRepeat &&  p->Servant->m_BM->checkData(*itExp))
			{
				vtSupplier.push_back(p->Servant);
				vtSpprSID.push_back(p->Servant->getSID());
			}
			p = p->Next;
		};
		lpMgr->lpSvt->m_Lock->off();
	
		//将提供者数据排序
		if(vtSupplier.size()>1)
		{
			sort(vtSupplier.begin(),vtSupplier.end());
		}

		for(vector<Servant *>::iterator it = vtSupplier.begin();it!= vtSupplier.end() && (it - vtSupplier.begin()) <= MAX_SUPPLIER ;it++)
		{
			if(lpSeg->t_ApplyTime == NULL)
				time(&lpSeg->t_ApplyTime);

			//向提供者发送数据申请指令
			//(*it)->ApplyData(*itExp);
			if(lpSeg->getPacketCount() == 0 && !(*it)->HasApplied(lpSeg->getSegID())) //从未申请过数据，并且未向此节点申请过
			{
				MsgApplyData *lpMsg = new MsgApplyData(lpMgr->dwSID);

				lpMsg->vtSegIDs.push_back(*itExp);

				(*it)->lpSktObj->SendTo(lpMsg);
			}
		}
		Sleep(1);
	}
}

/*
 *
 *	mCache(Membership)维护例程。
 *	定期向mCache中的成员发送Ping操作，并检测mCache中已经失效节点(超过一定的时间)，代其发送Leave消息。
 *	定期向一定的节点发送Connect消息，收到ConnectR消息后，加入Partnership(lpSvt)，必要时从Partnership移除等级差的节点
 *
*/
void ServntMgr::mCacheProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{

	ServntMgr *lpMgr = (ServntMgr *)dwUser;
	if(lpMgr->mCache.empty() || lpMgr->mCache.size () == 1)
		return;
	time_t tCurr;
	time(&tCurr);
	
	int iMaxRun = 5;
	
	//发送ping
	lpMgr->lpCacheLock->on();
	
	for(vector<ServantInfo*>::iterator it = lpMgr->mCache.begin(); it != lpMgr->mCache.end() ; it++)
	{
		ServantInfo* info = *it;
		if(!info->bIsMySelf && NULL != info->lpSktObj && info->lpSktObj->tLastPing != tCurr)
		{
			MsgPing *lpMsg = new MsgPing();

			lpMsg->dwSID = lpMgr->dwSID;
			lpMsg->dwIP = lpMgr->localAddr.sin_addr.s_addr;
			lpMsg->wPort = lpMgr->localAddr.sin_port;

			lpMsg->ttl = TTL;
			lpMsg->hops = HOPS;
			lpMsg->senderSID = lpMgr->dwSID;
			info->lpSktObj->SendTo(lpMsg);
			info->lpSktObj->tLastPing = tCurr;		

		}		
	}

	lpMgr->lpCacheLock->off();
	
	//检测所有节点的有效性
	lpMgr->lpCacheLock->on();
	
	vector<SocketObj*>	vtSkt_t;
	int iMaxConnect = 10;
	for(vector<ServantInfo*>::iterator it = lpMgr->mCache.begin()++ ;it !=  lpMgr->mCache.end(); )
	{
		if(!(*it)->bIsMySelf && (*it)->lpSktObj != NULL && difftime(tCurr,(*it)->lpSktObj->tLastUpdate) >= MAX_NODE_LIVE )
		{
			//lpMgr->Remove((*it).lpSktObj); //remove from mCache、ServantList and socketObjectList
		//	vtSkt_t.push_back((*it)->lpSktObj); //save to be-delete list
#ifdef _DEBUG
			if ((*it)->lpSktObj->sid == lpMgr->SrvSID)
			{
				//::_CrtDbgBreak();
			}
#endif
			//continue;
			//TODO:代发Leave消息
			vtSkt_t.push_back((*it)->lpSktObj);
			/*delete *it;*/
			it++;
		}
		else if (!lpMgr->bIsSrv)
		{
		
			if (NULL == (*it)->lpSktObj) //尚未Tcp连接
			{
				//建立Tcp连接
				if (IsHighID((*it)->dwSID) && iMaxConnect) //直接连接HighID
				{
					SOCKET skt = sys->createSocket(SOCK_STREAM);
					sys->setNoBlock(skt);
					SocketObj *lpSktObj = lpMgr->Append(skt);
					lpSktObj->addrlen = sizeof(lpSktObj->addr);
					memcpy(&lpSktObj->addr,&(*it)->addr,lpSktObj->addrlen);
					
					lpSktObj->bIsConnected = false;
					(*it)->lpSktObj = lpSktObj;
					lpSktObj->lpmCacheNode = *it;
					lpSktObj->lpServant = NULL;
					iMaxConnect--;
				} 
				else if (IsHighID(lpMgr->dwSID))//发送CallBack。如果本端也是LowID，则不发送callback
				{
					UdpCallBack *lpCallback = new UdpCallBack();
					lpCallback->SID=(*it)->dwSID; //向此servant发送callback
					lpCallback->Body.wCMD = PXY_CALLBACK;
					lpCallback->Body.dwSID = lpMgr->dwSID;
					//IP地址
					lpCallback->Body.dwIP =lpMgr->localAddr.sin_addr.s_addr;
					lpCallback->Body.wPort = lpMgr->localAddr.sin_port;

					lpCallback->Body.CHID = lpMgr->dwChID;
					lpMgr->SendToProxySrv(lpCallback);
					iMaxConnect--;
				}
			}
			it++;
		}
		else if (NULL==(*it)->lpSktObj)
		{
			delete *it;
			it=lpMgr->mCache.erase(it);
		}
		else
		{
			it++;
		}
	}
	lpMgr->lpCacheLock->off();


	for(vector<SocketObj*>::iterator it = vtSkt_t.begin() ; it!=vtSkt_t.end();it++)
	{
		lpMgr->Remove(*it);
	}
	vtSkt_t.clear();

	

	
	//发送Connect消息
	if(lpMgr->mCache.size() == 1)
		return;

	iMaxRun = 9;
	
	sort(lpMgr->mCache.begin(),lpMgr->mCache.end()); //对mCache进行排序


	//自动断开低效节点
	if (lpMgr->lpSvt->Count >= MAX_SERVANT_COUNT && static_cast<int>(lpMgr->mCache.size()) > MAX_SERVANT_COUNT) //已经连接的Servant数量超过最大Servant量，执行断开操作
	{
		int iMaxCount = MAX_SERVANT_COUNT-MAX_SEND_CONNECT;
		while (lpMgr->lpSvt->Count > iMaxCount) //断开5个
		{
			ServantInfo* lpInfo = lpMgr->mCache.at(lpMgr->lpSvt->Count);
			if (NULL == lpInfo->lpSktObj)
			{
				iMaxCount--;
			}
			else
			{
				//lpMgr->Remove(lpInfo->lpSktObj); //断开连接的最后一个
				shutdown(lpInfo->lpSktObj->skt,SD_BOTH);
				Sleep(20);
				closesocket(lpInfo->lpSktObj->skt);
				lpMgr->Remove(lpInfo->lpSktObj);
				//lpInfo->dwRecvCount = lpInfo->dwSentCount = 0;
				//lpInfo->lpSktObj->pStatistics.Init();
				iMaxCount--;
			}
		}
	}

	if(lpMgr->bIsSrv)
		return;


	//申请新连接
	
	lpMgr->lpCacheLock->on();

	vector<DWORD> vtCnt;
	for( int i = 0;i < MAX_SEND_CONNECT && lpMgr->lpSvt->Count+i < static_cast<int>(lpMgr->mCache.size()) && iMaxRun > 0 ; i++)
	{
		//int iLoc = lpMgr->randInt(1,static_cast<int>(lpMgr->mCache.size() - 1));
		ServantInfo* info =NULL;
		try
		{
			info= lpMgr->mCache.at(lpMgr->lpSvt->Count+i);
		}
		catch (std::out_of_range *)
		{
			break;
		}
		
		

		if(info->lpSktObj == NULL || info->lpSktObj->lpServant != NULL || find(vtCnt.begin(),vtCnt.end(),info->dwSID) != vtCnt.end())
		{
			iMaxRun--;
			continue;
		}
		MsgConnect *lpMsg = new MsgConnect();

		lpMsg->dwSID = lpMgr->dwSID;
				

		info->lpSktObj->SendTo(lpMsg);

		vtCnt.push_back(info->dwSID);
		iMaxRun--;
	}

	lpMgr->lpCacheLock->off();
}

//////////////////////////////////////////////////////////////////////////

char lpBm[BM_SIZE] ;

bool sendExBM(Servant* p,DWORD_PTR dwUser)
{
 	ServntMgr *lpMgr = (ServntMgr*)dwUser;
	if(NULL == lpBm)
		return false;
	
	if(p->getSID() != lpMgr->dwSID && lpMgr->SrvSID !=  p->getSID() ) //第一个节点是自身，不可以发送ExchangeBufferMap消息给自己
	{
	
		MsgExBM *lpExBM = new MsgExBM();
		lpExBM->dwSID = lpMgr->dwSID;
		lpExBM->lpBM = lpBm;
		assert(lpExBM->lpBM != NULL);
		p->lpSktObj->SendTo(lpExBM);
	}
	return true;
}

/*
*
*	Exchange BM例程
*
*
*/
void ServntMgr::ExBMProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	ServntMgr *lpMgr = (ServntMgr*)dwUser;
	
	char *lpBm_t = lpMgr->lpBuf->m_lpBM->toBytes();
	if (NULL == lpBm_t)
	{
		return;
	}
	memcpy(lpBm,lpBm_t,BM_SIZE);

	lpMgr->lpSvt->processSevants_s(sendExBM,dwUser);
}
/************************************************************************/
/* Proxy Hello，只有是LowID时才启动                                     */
/************************************************************************/
void ServntMgr::PrxHelloProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	UdpHello *lpMsg = new UdpHello();
	ServntMgr*me = (ServntMgr*)dwUser;
	lpMsg->SID = me->dwSID;
	me->SendToProxySrv(lpMsg);
}

void ServntMgr::SyncIDProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	ServntMgr *lpMgr = (ServntMgr*)dwUser;
	if (!lpMgr->bIsRunning || lpMgr->bIsSrv)
	{
		return;
	}
	static WORD wSeqID =0;
	if (wSeqID==0xffff)
	{
		wSeqID = 0;
	}
	wSeqID++;
	MsgSyncID *lpMsg = new MsgSyncID();
	lpMsg->wSeqID = wSeqID;
	lpMsg->dwSID = lpMgr->dwSID;
	lpMsg->dwChID=lpMgr->dwChID;

	//lpMgr->lpSrvSkt->SendTo(lpMsg);
	lpMsg->toBytes();
	sockaddr_in to;
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = IniUtil::getServerIP();
	to.sin_port = IniUtil::getServerPort();

	memset(to.sin_zero,0,sizeof(to.sin_zero));
	int ret = sendto(lpMgr->m_UdpSkt,lpMsg->lpBuf,lpMsg->bufLen,0,(sockaddr*)&to,sizeof(to));
	if (ret == SOCKET_ERROR)
	{
#ifdef _DEBUG
		cout<<"Can't Send SyncID:" << GetLastError() << endl;
#endif
	}
	delete lpMsg;
}



/************************************************************************/
/* 发送UDP消息到ProxySrv服务器                                          */
/************************************************************************/
void ServntMgr::SendToProxySrv(MessageBase *lpMsg)
{
	static sockaddr_in prxAddr;
	static int addrlen = sizeof(sockaddr_in);
	static WORD wSeq =0;
	if (wSeq == 0xffff)
	{
		wSeq =0;
	}
	wSeq++;
	if (prxAddr.sin_family!=AF_INET)
	{
		prxAddr.sin_family=AF_INET;
		prxAddr.sin_addr.s_addr = IniUtil::getProxySrvIP();
		prxAddr.sin_port = IniUtil::getProxySrvPort();
		memset(prxAddr.sin_zero,0,sizeof(prxAddr.sin_zero));
	}
	lpMsg->wSeqID = wSeq;
	lpMsg->toBytes();
	sendto(m_UdpSkt,lpMsg->lpBuf,lpMsg->bufLen,0,(sockaddr*)&prxAddr,addrlen);

	delete lpMsg;
}

/************************************************************************/
/* Udp服务程序                                                       */
/************************************************************************/
int ServntMgr::UdpWrkThread(ThreadInfo *lpInfo)
{
	const int bufLen = 128;
	
	ServntMgr* me = ((ThreadData*)lpInfo->data)->lpMgr;
		
	char* buf = new char[bufLen];
	int ret;
	sockaddr_in srvAddr,addr;
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = IniUtil::getLocalPort();
	srvAddr.sin_addr.s_addr  = htonl(INADDR_ANY);
	memset(&(srvAddr.sin_zero), '\0', 8);
	bind(me->m_UdpSkt,(sockaddr*)&srvAddr,sizeof(srvAddr));

	int fromLen = sizeof(srvAddr);
	while (me->bIsRunning)
	{
		Sleep(10);
		ret = recvfrom(me->m_UdpSkt,buf,bufLen,0,(sockaddr*)&addr,&fromLen);
		if (ret == SOCKET_ERROR || ret == 0)
		{
			DWORD dwErr = GetLastError();
			continue;
		}
		WORD wCmd;
		memcpy(&wCmd,buf,2);

		switch(wCmd)
		{
		case CMD_SYNC_ID:
			{
				MsgSyncID* lpReq = new MsgSyncID();
				lpReq->fromBytes(buf,ret);

				MsgSyncIDR* lpResp = new MsgSyncIDR();
				lpResp->dwSegID = me->lpBuf->getStartID();
				lpResp->dwChID=me->dwChID;
				
				lpResp->wSeqID = lpReq->wSeqID;
				lpResp->toBytes();

				int ret =sendto(me->m_UdpSkt,lpResp->lpBuf,lpResp->bufLen,0,(sockaddr*)&addr,fromLen);
				if (SOCKET_ERROR == ret)
				{
					DWORD dwErr = GetLastError();
#ifdef _DEBUG
					cout << "Send SyncID Resp Error:" << dwErr << endl;
#endif
				}
				SocketObj *lpObj = me->Find(lpReq->dwSID);
				if (NULL!=lpObj)
				{
					lpObj->SyncID(lpResp->dwSegID);
				}
				Sleep(1);
				delete lpReq;
				delete lpResp;

			}
			break;
		case CMD_SYNC_ID_R:
			{
				me->m_UdpIsOK=true;
				MsgSyncIDR *lpMsg = new MsgSyncIDR();
				lpMsg->fromBytes(buf,ret);
				SEGMENTID cStartID = lpMsg->dwSegID;//me->lpBuf->getStartID();
				
				DoSyncIDProcess(cStartID,me);
				
				delete lpMsg;
			}
			break;
		case PXY_HELLO://nothing to do .the proxy server will handle this command
			break;
		case PXY_HELLO_R:
			{
				UdpHelloR *lpMsg = new UdpHelloR();
				lpMsg->fromBytes(buf,ret);
				for(vector<UdpCmd*>::iterator it = lpMsg->vtCmds.begin() ; it != lpMsg->vtCmds.end() ;it++)
				{
					//如果已经连接，则忽略，否则发出一个connect指令
					ServantInfo *info = NULL;
					me->lpCacheLock->on();
					for(vector<ServantInfo*>::iterator it2 = me->mCache.begin() ; it2 != me->mCache.end(); it2++)
					{
						if((*it2)->dwSID == (*it)->dwSID)
						{
							info = *it2;
							break;
						}
					}
					me->lpCacheLock->off();
					if(NULL == info)
					{
						//不存在与mCache中
						info = new ServantInfo();
						info->dwSID = (*it)->dwSID;
						info->addr.sin_family = AF_INET;
						info->addr.sin_addr.s_addr = (*it)->dwIP;
						info->addr.sin_port = (*it)->wPort;
						memset(info->addr.sin_zero,0,sizeof(info->addr.sin_zero));

						SOCKET skt = sys->createSocket(SOCK_STREAM);
						sys->setNoBlock(skt);

						SocketObj *lpSktObj = me->Append(skt);// new SocketObj(true);

						lpSktObj->addrlen = sizeof(lpSktObj->addr);
						memcpy(&lpSktObj->addr,&info->addr,lpSktObj->addrlen);
						

						lpSktObj->lpmCacheNode = info;
						info->lpSktObj = lpSktObj;
						lpSktObj->lpServant = NULL;
						me->lpCacheLock->on();
						me->mCache.push_back(info);
						me->lpCacheLock->off();
					}
					
					if(!info->lpSktObj->bIsConnected) //尚未连接
					{
						MsgConnect *lpCnn = new MsgConnect();
						lpCnn->dwSID = me->dwSID;
						info->lpSktObj->SendTo(lpCnn);
					}
				}
				delete lpMsg;
			}
			break;
		case PXY_CALLBACK://nothing to do .the proxy server will handle this command
			break;
		case PXY_CALLBACK_R:
			{

			}
			break;
		case PXY_LEAVE: //TODO:save id to reuseable ids
			{
				
			}

		}
		
					
	}
	delete[] buf;
	return -3;
}
void ServntMgr::DoSyncIDProcess(SEGMENTID cStartID,ServntMgr *me)
{
	if (me->lpBuf->wServerCurrSegID > cStartID)
	{

		//ReConnect
		me->Restart();

	}
	else
	{
		me->lpBuf->wServerCurrSegID = cStartID;
		me->lpSvt->processSevants_s(ServntMgr::SyncIDHelper,cStartID);

		me->lpBuf->SetPosition(cStartID); //调整StartID
	}

	me->lpBuf->setTimer();
	//lpSktObj->lpServant->SyncID(lpMsg->dwSegID);	//adjust the applied data

}
//////////////////////////////////////////////////////////////////////////
// 监控线程
//////////////////////////////////////////////////////////////////////////
void ServntMgr::MonitorProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	ServntMgr *lpMe = (ServntMgr*)dwUser;
	if (lpMe->bIsRunning && ( lpMe->mCache.empty() || static_cast<int>(lpMe->mCache.size()) <2 ))
	{
		lpMe->Restart();
	}
}