/*
	
	一些关键算法说明

	1、作为客户端，每隔20秒发起一下SyncID消息，接收到SyncID Resp后，检查，如果服务端的ID与本地ID的差超过60%，
则将本地的消息同步为服务器上的ID
	2、TimeStep例程检测主缓冲区的可用空间，在可用空间小于10%后释放掉一个Segment，为保证连续性，TimeStep例程可以每秒执行一次
	3、当接收到15个Segment(15秒)后启动播放器，进行本地播放
*/

#pragma once
#include "Buffer.h"
#include "Protocol.h"
#include "Servant.h"
#include "messages.h"
#include <vector>
#include <list>
#include "sys.h"
#include "Queue.h"
#include "FileDeliver.h"
#include "HttpDeliver.h"
#include "SourcePin.h"

#define MAX_SUPPLIER 4	//每个Segment最大的提供者
//#define PING_COUNT	4	//同时向多少个客户机发送Ping消息
#define TTL		6		//TTL
#define HOPS	0		//hops
#define MAX_NODE_LIVE	1*60	//mCache中最大的存活时间，单位 秒
#define	MAX_SEND_CONNECT 5	//最大发送的Connect消息
#define SCHEDULER_TIMER	199	//调度器执行时间间隔，单位 毫秒
#define MCACHE_TIMER	9 * 1000	//mCache_Proc执行间隔时间
#define EXBM_TIMER		6 * 1000	//Exchange BM Proc
#define SYNC_ID_TIMER	25 * 1000 //同步ID例程执行时间
#define MAX_LOWID		0x1000000 //最大的LowID
#define IsHighID(id)	(id > MAX_LOWID) //是否是HighID
#define IsLowID(id)		(id <= MAX_LOWID)
#define PRX_HELLO_TIMMER 10 * 1000 //向ProxyServer发送Hello的时间间隔

#define MAX_MCACHE_COUNT	120	//最大的mcache数量
#define MAX_SERVANT_COUNT	32	//最大的servant数量
/**********************************************************************
 *
 * 线程对象
 *
 *********************************************************************/

class ServntMgr;

class  ThreadData
{
	friend class ServntMgr;
public:
	ThreadData(ServntMgr *lpmgr)
	{
		m_Lock = new WLock();
		lpQueue = new Queue();

		lpMgr = lpmgr;
	}
	~ThreadData()
	{
		delete m_Lock;
		delete lpQueue;
	}

public:

	WLock *m_Lock;
	ServntMgr *lpMgr;
	vector<SocketObj*> vtSocket; //线程中SocketObj实例列表
	Queue	*lpQueue;				//接收消息队列
	bool bRunAsTCP;


	bool Append(SOCKET skt,bool isListening,SocketObj **lpSktObj)
	{
		if(vtSocket.size() == FD_SETSIZE )
		{
			return false;
		}
		m_Lock->on();
		
		SocketObj *lpObj = new SocketObj(bRunAsTCP);
		lpObj->bIsListening = isListening;
		lpObj->lpServant= NULL;
		lpObj->skt = skt;
		lpObj->lpmCacheNode = NULL;

		vtSocket.push_back(lpObj);

		*lpSktObj = lpObj;

		m_Lock->off();
		return true;
	}

	bool Append(SOCKET skt,SocketObj **lpSktObj)
	{
		return Append(skt,false,lpSktObj);
	}
	
	bool Remove(SocketObj *lpSktObj)
	{
		//remove from vtSocket
		bool bFound = false;
		m_Lock->on();
		for(vector<SocketObj *>::iterator it2 = vtSocket.begin(); it2 !=vtSocket.end() ; it2++)
		{
			if((*it2) == lpSktObj)
			{
				//found
				vtSocket.erase(it2);
				bFound = true;
				break;
			}
		}
		m_Lock->off();
		return bFound;
	}
	

	SocketObj* Find(PEERID _sid)
	{
		SocketObj *lpRet = NULL;
		m_Lock->on();
		for(vector<SocketObj*>::iterator it = vtSocket.begin() ; it != vtSocket.end() ;it++)
		{
			
			if((*it)->sid == _sid && (*it)!= 0)
			{
				lpRet = *it;
				break;
			}
		}
		m_Lock->off();
		return lpRet;
	}
} ;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ServntMgr
{
public:
	ServntMgr(void);
	~ServntMgr(void);

	//启动。作为客户端
	void Start(char* szIP,WORD wPort,DWORD ChID);
	//启动。作为服务器
	void srv_Start(DWORD ChID);
	//停止
	void Stop(bool bIsRestart = false,int ErrorCode = 40);
	PEERID dwSID;						//本端的ID
	PEERID SrvSID;						//服务器端的SID
	//频道ID
	DWORD dwChID;
	
	sockaddr_in	localAddr,serverAddr;	//本地的ip地址
	/************************************************************************/
	/* 状态接口 Level:状态级别。1:General information;2:Error;3:trigger Event*/
	/************************************************************************/
	 void (__stdcall * StatusPin)(int StatusCode,int Detail);
private:

	vector<ServantInfo*> mCache;			//存入覆盖网的局部信息
	WLock *lpCacheLock;						//mCache

	bool	bIsRunning	;				//是否在运行
	Buffer *lpBuf;						//主Buffer对象
	//vector<ThreadInfo> m_Threads;		//线程池
	WLock *lpThreadsLock;				//线程池锁
	bool	bIsSrv;						//是否作为服务端运行
	bool	bRunAsTCP;					//是否以TCP运行
	//SocketObj *lpSrvSkt;				//连接Server的Socket对象，只有运行于客户端模式时，才有定义
	vector<DWORD> vtLocalIps;			//本机IP地址

	/************************************************************************/
	/*                                                             */
	/************************************************************************/
	SourcePin *lpSrcPin;
	
	/************************************************************************/
	/* 数据接收工作线程。在initMgr函数启动
	/* 只负责数据接收工作，如果在本机的Socket列表中没有相关记录，会自动将发送方添加到Socket列表，并创建SocketObj对象。接收到的数据存入lpInfo->lpData->lpQueue，由对应的workThreadHelperFunc负责处理。*/
	/************************************************************************/
	static  int workThreadFunc(ThreadInfo *lpInfo);

	/************************************************************************/
	/*数据处理工作线程的辅助线程。在initMgr函数启动                         */
	/*负责从对应的lpInfo->lpData->lpQueue读取数据并处理
	/************************************************************************/
	static	int workHelperThreadFunc(ThreadInfo *lpInfo);
	
	/************************************************************************/
	/*Udp处理线程/
	/************************************************************************/
	static int UdpWrkThread(ThreadInfo *lpInfo);


	/************************************************************************/
	/* Client端的主线程由Start启动                                          */
	/************************************************************************/
	static int CltWrkThread(ThreadInfo *lpInfo);

	//Socket队列相关
	SocketObj* Append(SOCKET ,bool bIsListening);
	SocketObj* Append(SOCKET);
	SocketObj* Find(PEERID );
	
	//void Remove(SOCKET);
	void Remove(SocketObj*);

	SOCKET m_skt;					//主Socket
	SOCKET m_UdpSkt;				//到主服务器的UDP Socket
	
	FileDeliver *lpFileDeliver;
	HttpDeliver *lpHttpDeliver;
	

	ServantList *lpSvt;					//ServantList，已经连接的Node列表。其lpSktObj->lpmCacheNode!=null
	vector<ThreadInfo *> vtThreads;		//Socket线程池

	void initMgr();			//初始化Mgr
	void SendToProxySrv(MessageBase *);

	//同步ID辅助函数
	static bool SyncIDHelper(Servant*,DWORD_PTR);
	
	/************************************************************************/
	/* 调度程序例程
	/* 只有作为客户端运行时启动。是关键的执行例程                           */
	/************************************************************************/
	void static CALLBACK schedulerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	//mCache信息交换例程
	void static CALLBACK mCacheProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	//ExchangeBM信息交换例程
	void static CALLBACK ExBMProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	//SyncID例程
	void static CALLBACK SyncIDProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	void static  DoSyncIDProcess(SEGMENTID cStartID, ServntMgr *lpMe);
	
	//PrxyHello线程。只有在LowID时才启动。
	void static CALLBACK PrxHelloProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	//监控线程
	
	void static CALLBACK MonitorProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
		
	UINT uintScheduler,uintmCache,uintExBM,uintSyncID,uintPrxHello,uintMonitor;///,uintSendMsg;

	bool m_UdpIsOK;
	void startScheduler()	//启动调度程序
	{
		uintmCache = timeSetEvent(MCACHE_TIMER,1,ServntMgr::mCacheProc,(DWORD_PTR)this,TIME_PERIODIC); //5秒启动一次

		uintExBM = timeSetEvent(EXBM_TIMER,1,ServntMgr::ExBMProc,(DWORD_PTR)this,TIME_PERIODIC);	//ExchangeBM调度

		MMRESULT ret;
		if ( !bIsSrv ) //client
		{
			uintSyncID = timeSetEvent(SYNC_ID_TIMER,1,ServntMgr::SyncIDProc,(DWORD_PTR)this,TIME_PERIODIC); //SyncID
			lpBuf->setTimer();

			ret = timeSetEvent(SCHEDULER_TIMER,1,ServntMgr::schedulerProc,(DWORD_PTR)this,TIME_PERIODIC); //Scheduler
			uintScheduler = ret;
			if (IsLowID(dwSID))
			{
				uintPrxHello = timeSetEvent(PRX_HELLO_TIMMER,1,ServntMgr::PrxHelloProc,(DWORD_PTR)this,TIME_PERIODIC);
			}
			uintMonitor=timeSetEvent(10*1000,1,ServntMgr::MonitorProc,(DWORD_PTR)this,TIME_PERIODIC); //monitor
		}
		else //server
		{
			uintScheduler = NULL;
		}
	
	};
	void stopScheduler(bool isRestart = false)	//停止调度程序
	{
		if(NULL != uintScheduler)
			timeKillEvent(uintScheduler);
		timeKillEvent(uintmCache);
		timeKillEvent(uintExBM);
		
		if(!bIsSrv)
		{
			timeKillEvent(uintMonitor);
		}
		timeKillEvent(uintSyncID);
		
		
		if (uintPrxHello>0 )
		{
			timeKillEvent(uintPrxHello);
			uintPrxHello =0;
		}
	};

	//void startHttpd();		//启动内置http服务器
	//void stopHttpd();		//停止内置http服务器
	
	
	void Restart()//重新启动。作为客户端时有定义
	{
		lpBuf->Reset(0);
		lpCacheLock->on();
		for(vector<ServantInfo*>::iterator it = mCache.begin();it != mCache.end();it++)
		{
			if (NULL != (*it)->lpSktObj)
			{
				(*it)->lpSktObj->wSndSegID= 0;
				(*it)->lpSktObj->ClearSendBuf();
			}
		}
		lpCacheLock->off();

		lpHttpDeliver->Clear();

		//重新发送Connect

		//投递一个加入请求

		MsgJoin *lpMsg = new MsgJoin(dwSID,dwChID);


		SOCKET skt ;
		if(bRunAsTCP)
			skt = sys->createSocket(SOCK_STREAM);
		else
			skt = m_skt;


		SocketObj *lpSrvSkt = Append(skt); // lpSrvSkt's lpmCacheNode is NULL,need to fix
		memcpy(&lpSrvSkt->addr,&serverAddr,sizeof(serverAddr));

		lpSrvSkt->addrlen = sizeof(serverAddr);
		lpMsg->vtIP = vtLocalIps;
		lpMsg->wPort =localAddr.sin_port;

		lpSrvSkt->SendTo(lpMsg);
		
	}
	void setStatus(int StatusCode,int Detail)
	{
		if (StatusPin != NULL)
		{
			StatusPin(StatusCode,Detail);
		}
	}
	void setStatus(int StatusCode)
	{
		setStatus(StatusCode,0);
	}
public:
	//void (* lpRestart)(void);
	//产生随机数（整数），位于区间[a,b]或者[b,a]
	static int randInt(int a, int b)
	{
		if(a<b)
			return a + rand()%(b-a+1);
		else if(a>b)
			return b + rand()%(a-b+1);
		else // a==b
			return a;
	};

private:
typedef	struct SocketInfo_TAG
	{
		SOCKET skt;
		sockaddr_in addr;
		int addrlen;
	} SocketInfo;
};
