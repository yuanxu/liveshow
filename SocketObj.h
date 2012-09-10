#pragma once
#include "sys.h"
#include "messages.h"
#include <vector>
#include "common.h"
#include <queue>

/**********************************************************************
 *
 * Socket对象
 *
 *********************************************************************/
#define SEND_WND_SIZE	16	//发送窗口的尺寸
class Servant;
class Queue;

using namespace std;
typedef struct StatisticsTag
{
public :
	volatile LONG BytesRead,
              BytesSent,
              StartTime,
              BytesReadLast,
              BytesSentLast,
              StartTimeLast;
	volatile LONG AverageSentKBps,AverageReadKBps,CurrentSentKBps,CurrentReadKBps;

	//initialize 
	void Init()
	{
		BytesRead = BytesSent,BytesReadLast,BytesSentLast = 0;
		StartTimeLast = StartTime = GetTickCount();
		AverageSentKBps = 0,AverageReadKBps = 0,CurrentSentKBps = 0,CurrentReadKBps = 0;
	}

	//Calculate statistic data
	void Calculate()
	{
		ULONG   tick, elapsed;

		tick = GetTickCount();

		elapsed = (tick - StartTime) / 1000;

		if (elapsed == 0)
			return;
		
		//AverageSentKBps = BytesSent / elapsed /1024;
		InterlockedExchange(&AverageSentKBps,BytesSent / elapsed /1024);
		//printf("Average BPS sent: %lu [%lu]\n", bps, gBytesSent);

		//AverageReadKBps = BytesRead / elapsed /1024;
		InterlockedExchange(&AverageReadKBps,BytesRead / elapsed /1024);
		//printf("Average BPS read: %lu [%lu]\n", bps, gBytesRead);

		elapsed = (tick - StartTimeLast) / 1000;

		if (elapsed == 0)
			return;

		//CurrentSentKBps = BytesSentLast / elapsed /1024;
		InterlockedExchange(&CurrentSentKBps, BytesSentLast / elapsed /1024);
		//printf("Current BPS sent: %lu\n", bps);

		//CurrentReadKBps = BytesReadLast / elapsed /1024;
		InterlockedExchange(&CurrentReadKBps,BytesReadLast / elapsed /1024);
		//printf("Current BPS read: %lu\n", bps);

	    

		InterlockedExchange(&BytesSentLast, 0);
		InterlockedExchange(&BytesReadLast, 0);

		StartTimeLast = tick;
	}

} Statistics;

class SocketObj
{
public:
	SocketObj(bool);
	~SocketObj(void);

	SOCKET skt;
	Servant* lpServant;
	bool bIsListening ;
	ServantInfo* lpmCacheNode;
	sockaddr_in addr;
	int addrlen;
	bool bCanWrite;
	bool bIsConnected;
	bool bRunAsTCP;
	time_t tLastUpdate,tLastPing;
	DWORD dwLastSendTick;
	PEERID sid;
	DWORD dwChID;
	
	 WLock *lpSendLock ;//= new WLock();	
	 			//发送滑动窗口
	 deque<MessageBase *> m_sendBuf; //发送队列

	 Statistics pStatistics;
	 static Statistics gStatistics;
	 
	 SEGMENTID wSndSegID;		//正在发送的SegmentID。如果当前处理的不是PushData指令，则此值为0
private:
	//char *lpSndBuf;		//发送缓冲 
	//int iSizeToSnd;		//待发送字节
	int iSndOffset;		//已发送字节
	int iSndID;			//正在发送的消息ID

#define iSizeOfRecvBuf (32 * 1024) 	//接收缓冲区大小。缓冲区采用重用机制	
	char lpRecvBuf[iSizeOfRecvBuf];	//接收缓冲		32kb

	int	iSizeToRecv;	//待接收字节
	int	iRecvOffset;	//已接收字节
	
	//正在发送的消息
	MessageBase *lpMsg ;
	ReliableMessageBase *lpRlbMsg;
public:

	int SendTo();	//发送数据
	void SendTo(MessageBase *lpMsg);
	WORD _SeqID;
	inline WORD getNextSeqID()
	{
		
		if(0xffff == _SeqID)
		{
			_SeqID = 0;
		}
		return _SeqID++;
	};

	inline void SendTo_s()
	{
	
		lpSendLock->on();
		try
		{
			while (!m_sendBuf.empty() && bCanWrite)
			{
				if(0 == SendTo()) //WSAWOULDBLOKING
					break;
				Sleep(1);
			}
		}
		catch (SocketException* e)
		{
			lpSendLock->off();
			throw e;
		}
		
		lpSendLock->off();
	};

	void EraseMsg(WORD wSeqID);
	int Receive(Queue *lpQueue); //==0 must close
	void SyncID(SEGMENTID dwSegID);	//调整Local Buffer时执行。将超过期限的数据移除。
	void ClearSendBuf()
	{
		lpSendLock->on();
		while (!m_sendBuf.empty())
		{
			delete m_sendBuf.front();
			m_sendBuf.pop_front();
		}
		lpSendLock->off();
	}
};
