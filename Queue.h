#pragma once
#include "common.h"
#include "Messages.h"
#include "stdafx.h"
#include "sys.h"
#include <queue>

#define MAX_QUEUE_SIZE 256	//最大队列尺寸

using namespace std;

typedef struct QueueData_TAG
{
	char* buf;
	int len;
	int capacity ;
	SocketObj *lpSktObj;

} QueueData;


class SocketObj;

class Queue
{
private:
	WLock *m_Lock;
	deque<QueueData*> qDatas;
public:
	Queue(void)
	{
		m_Lock = new WLock();	
	};
	~Queue(void)
	{
		while (!qDatas.empty())
		{
			delete[] qDatas.front()->buf;
			delete qDatas.front();
			qDatas.pop_front();
		}
		delete m_Lock;
		
	}
	
public:
	inline void Pop(char **p,int *len,SocketObj **lpSktObj)
	{

		if (qDatas.empty())
		{
			*len = -1;
			return;
		}
		m_Lock->on();
		QueueData *lpData = qDatas.front();
		
		*p=lpData->buf;
		*len = lpData->len;
		*lpSktObj =lpData->lpSktObj;
		qDatas.pop_front();
		m_Lock->off();
		
	};
	inline void	Push(const char *p,const int len,SocketObj *lpSktObj)
	{
		m_Lock->on();
		
		if (qDatas.size()>MAX_QUEUE_SIZE)
		{
			delete[] qDatas.front()->buf;
			delete qDatas.front();
			qDatas.pop_front();
		}

		WORD wCMD,wSeq ;
		memcpy(&wCMD,p,2);
		memcpy(&wSeq,p+2,2);
		
		//if(wCMD == CMD_PING || wCMD == CMD_APPLY_PACKET || wCMD == CMD_EXCHANGE_BM)
		//{
		//	if(0 == wSeq || wSeq>0xfff0) //多加几个修正的机会
		//	{
		//		lpSktObj->wCtlSeqID = 0;
		//	}
		//	else if (wSeq <= lpSktObj->wCtlSeqID && lpSktObj->wCtlSeqID - wSeq < MAX_QUEUE_SIZE /10 )//当前收到的消息小于已处理消息，且他们的差小于1/10的最大消息队列大小
		//	{
		//		m_Lock->off();
		//		return;
		//	}
		//	
		//}
		
		QueueData *lpData = new QueueData();
		lpData->buf = new char[len];
		memcpy(lpData->buf,p,len);
		lpData->len = len;
		lpData->lpSktObj = lpSktObj;
		qDatas.push_back(lpData);

		m_Lock->off();
	};
	int Size()
	{
		return qDatas.size();
	}
	void Remove(SocketObj *lpSktObj)
	{
		m_Lock->on();
		for(deque<QueueData*>::iterator it = qDatas.begin(); it != qDatas.end();)
		{
			if ((*it)->lpSktObj == lpSktObj)
			{
				delete[] (*it)->buf;
				delete *it;
				it = qDatas.erase(it);
			} 
			else
			{
				it++;
			}
		}
		m_Lock->off();
	}
};
