/************************************************************************/
/* Http 下发程序
/* 算法：提供一个15或20个Segment的下发缓冲，当有新的http客户端连上来后，将这些缓存的Segment全部下发
/*		，之后正常取到Segment时，一方面存入下发缓冲，另一方面存入http客户端缓冲。
/************************************************************************/

#pragma once
#include "ideliver.h"
#include <vector>
#include <queue>
#include "sys.h"
#include "Segment.h"
#include "common.h"

#define READY_TO_PLAY 15
#define MAX_CACHE_SIZE (READY_TO_PLAY*1.5)

typedef struct tagHttpData 
{
	char*pData;
	int len;
}HttpData;

typedef struct tagHttpSocket 
{
	SOCKET skt;
	bool bHasGotHeader;
	bool bIsReady;
	bool bIsListener;
	deque<HttpData*> qDatas;

	void Deliver(const char* pSeg,int len =Segment::getSegmentSize())
	{
		HttpData *pData = new HttpData();
		pData->pData=new  char[len];
		pData->len=len;
		memcpy(pData->pData,pSeg,len);
		qDatas.push_back(pData);
		//qDatas.push(pData);
	}

	void Send()
	{

		if (qDatas.empty())
		{
			return;
		}

		int ret;
		
		HttpData *lpData =NULL;
		lpData = qDatas.front();
		ret = send(skt,lpData->pData,lpData->len,0);
		DWORD dwErr =GetLastError();
		if(SOCKET_ERROR == ret ) //发生错误　
		{
			if (dwErr != WSAEWOULDBLOCK)
			{
#ifdef _DEBUG
				cout << "Http Deliver Deliver Segment ERROR" << dwErr;
#endif
				if (NULL != lpData)
				{
					delete[] lpData->pData;
					delete lpData;
					qDatas.pop_front();
				}

				throw new SocketException(dwErr);	
			}
			else if(dwErr==WSAEWOULDBLOCK)
			{
				return;
			}
		}		
	
		qDatas.pop_front();
		delete[] lpData->pData;
		delete lpData;
		
	}
		
} HttpSocket;


using namespace std;
class HttpDeliver :
	public IDeliver
{
private:
	char* lpHeader;
	int headerLen;
	bool bIsRunning;

	SOCKET sktListener;//监听端口
	vector<HttpSocket> vtSkts; //连接过来的客户端
	WLock *lpLock;
	vector<char*> vtBuf; //主播放/下发缓冲

	ThreadInfo *lpWorkThread,*lpSendThread;
	static int WorkThread(ThreadInfo *);
	static int SendThread(ThreadInfo *);
	
	void setStatus(int ErrorCode,int Detail)
	{
		if (StatusPin != NULL)
		{
			StatusPin(ErrorCode,Detail);
		}
	}
	bool bPlayerStared ;
public:
	HttpDeliver(void);
	~HttpDeliver(void);
	/************************************************************************/
	/* 状态接口 Level:状态级别。1:General information;2:Error;3:trigger Event*/
	/************************************************************************/
	void (__stdcall *StatusPin)(int StatusCode,int detailCode);

	void Deliver(Segment*,bool);
	void DeliverHeader(char *,int);
	void ImmediateDeliver(Segment* p,bool b)
	{
		if (p->getSegID() == dwExpectSegmentID || dwExpectSegmentID==0)
		{
			Deliver(p,true);
		}
	}
	void Clear()
	{
		for (vector<char*>::iterator it = vtBuf.begin();it!= vtBuf.end();)
		{
			delete[] *it;
			it=vtBuf.erase(it);
		}
		dwExpectSegmentID =0;
	}
	void Start();
	void Stop();
	int getBufferRate()
	{
		return vtBuf.size() / MAX_CACHE_SIZE * 100 +0.5;
	}
	static int randInt(int a, int b)
	{
		if(a<b)
			return a + rand()%(b-a+1);
		else if(a>b)
			return b + rand()%(a-b+1);
		else // a==b
			return a;
	};

};
