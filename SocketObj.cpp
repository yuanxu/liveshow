#include "StdAfx.h"
#include "SocketObj.h"
#include "Queue.h"
#include <algorithm>
#include "Servant.h"

using namespace std;

Statistics SocketObj::gStatistics ;

SocketObj::SocketObj(bool runAsTCP)
{
	lpSendLock = new WLock();
	bCanWrite = true;
	bIsConnected = false;
	bRunAsTCP = runAsTCP;
	bIsListening = false;
	tLastUpdate = time((time_t*)NULL);
	tLastPing = time((time_t*)NULL);
	dwLastSendTick =0;
	lpServant=NULL;
	lpmCacheNode =NULL;

	
	iSndOffset =iSizeToRecv = iSndOffset = 0;
	wSndSegID = 0;
	//lpRecvBuf = new char[iSizeOfRecvBuf];
	iRecvOffset =0;

	sid = 0;
	_SeqID = 0;
	
	lpMsg = NULL;
	lpRlbMsg = NULL;

	pStatistics.Init();
}


SocketObj::~SocketObj(void)
{
	
	static   struct   linger   lig;   
	lig.l_onoff=1;   
	lig.l_linger=0;   
	static   int   iLen=sizeof(struct   linger);   
	setsockopt(skt,SOL_SOCKET,SO_LINGER,(char   *)&lig,iLen);   

	int ret = shutdown(skt,SD_BOTH);
	Sleep(20);//
	ret = closesocket(skt);

	skt =INVALID_SOCKET;
	//delete[] lpRecvBuf;	
	lpSendLock->on();

	while ( !m_sendBuf.empty())
	{
		delete m_sendBuf.back();
		m_sendBuf.pop_back();
	}
		
	m_sendBuf.clear();

	lpSendLock->off();
	delete lpSendLock;
	lpSendLock = NULL;
}

void SocketObj::EraseMsg(WORD wSeqID)
{
	
	//TCP无需自己制作滑动窗口
	return;
}

void SocketObj::SendTo(MessageBase *p)
{
	
	lpSendLock->on();
	if(NULL != p)
		m_sendBuf.push_back(p);
	lpSendLock->off();
}


int SocketObj::SendTo()
{
	
	if (NULL != lpMsg)
	{
		if (lpMsg->wCMD == CMD_PUSH_DATA && ((MsgPushData*)lpMsg)->dwSegID < wSndSegID )  //数据已经过期
		{
			
			iSndOffset = 0;
			delete lpMsg;
			m_sendBuf.pop_front();

			lpMsg = NULL;
			lpRlbMsg = NULL;
			return -1; //数据过期时，告知外层SendTo_s()快速处理掉
		}
	}
	else
	{
		iSndID = 0;
		iSndOffset = 0;
		//wSndSegID = 0;
		lpMsg = m_sendBuf.front();
		if (lpMsg->bNeedConfirm)
		{
			lpRlbMsg = (ReliableMessageBase*)lpMsg;
		}
		else
		{
			lpRlbMsg =NULL;
		}

		if(lpMsg->wCMD < 0x8000)
			lpMsg->wSeqID = getNextSeqID();
		lpMsg->dwChID = dwChID;
		lpMsg->toBytes();

		if (lpMsg->wCMD == CMD_PUSH_DATA && ((MsgPushData*)lpMsg)->dwSegID < wSndSegID )  //数据已经过期
		{

			iSndOffset = 0;
			delete lpMsg;
			m_sendBuf.pop_front();

			lpMsg = NULL;
			lpRlbMsg = NULL;
			return -1; //数据过期时，告知外层SendTo_s()快速处理掉
		}
		iSndID = lpMsg->wSeqID;
		
	}

	//send
	int ret = 0;

	
	if(bRunAsTCP)
		ret = send(skt,lpMsg->lpBuf+iSndOffset,lpMsg->bufLen -  iSndOffset,0);
	else
		ret = sendto(skt,lpMsg->lpBuf,lpMsg->bufLen,0,(sockaddr*)&addr,addrlen);
	if(SOCKET_ERROR == ret )
	{
		//错误处理

		DWORD err = GetLastError();
		if (WSAEWOULDBLOCK != err)
		{		
			throw new SocketException(err);
		}
		else
		{
			bCanWrite = false; //设置为不可写，发送完成后，select函数会将些值设为可写。此时所有数据已经发出。
			ret =0;//no send
			return ret;
		}
	}

	//记录统计信息
	pStatistics.BytesSent+= ret;
	pStatistics.BytesSentLast += ret;
	pStatistics.Calculate();


	InterlockedExchangeAdd(&gStatistics.BytesSentLast ,ret);
	InterlockedExchangeAdd(&gStatistics.BytesSent ,ret);
	
		
	gStatistics.Calculate();
	
	if(CMD_PUSH_DATA ==  lpMsg->wCMD)
	{
		lpmCacheNode->dwSentCount++;
	}

		dwLastSendTick = GetTickCount();		
		time(&tLastUpdate);
		if (CMD_PING == lpMsg->wCMD)
		{
			time(&tLastPing);
		}

		
#if _DEBUG
		{
			char buf_t[512];
			sprintf(buf_t,"Send Command %x,SeqID %u",lpMsg->wCMD,lpMsg->wSeqID);
		}
#endif
		iSndOffset += ret;
		if (iSndOffset == lpMsg->bufLen)
		{	
			iSndOffset = 0;
			iSndID = 0;
			

			//通过tcp传输，所有数据都是可靠的，所有不需要反馈
			//if (!lpMsg->bNeedConfirm)
			//{
				// don't need replay ,just send it to the network
				delete lpMsg;
				lpMsg = NULL;
				lpRlbMsg = NULL;
				m_sendBuf.pop_front();
			//}
		}
	return ret;	
}

void SocketObj::SyncID(SEGMENTID dwSegID)
{
	wSndSegID = dwSegID;	
}

int SocketObj::Receive(Queue *lpQueue)
{
	int ret;
	if ( 0 == iSizeToRecv )
	{
		//memset(lpRecvBuf,0,sizeof(lpRecvBuf));
		ret = recv(skt,lpRecvBuf+iRecvOffset ,PTL_HEADER_SIZE-iRecvOffset,0);
		if (SOCKET_ERROR == ret && GetLastError() != WSAEWOULDBLOCK) // disconnected
		{
#ifdef _DEBUG
			std::cout << "Receive Error:" << GetLastError();
#endif
			throw new SocketException(GetLastError());
			
		}
		
		iRecvOffset += ret;

		pStatistics.BytesRead += ret;
		pStatistics.BytesReadLast += ret;

		InterlockedExchangeAdd(&gStatistics.BytesRead,ret);
		InterlockedExchangeAdd(&gStatistics.BytesReadLast,ret);
	} 
	if (iRecvOffset == PTL_HEADER_SIZE && 0==iSizeToRecv)
	{
		memcpy(&iSizeToRecv,lpRecvBuf  + 4,2); //read command size
		assert(iSizeToRecv - PTL_HEADER_SIZE <= iSizeOfRecvBuf);	
	}
	if (iSizeToRecv!=iRecvOffset && iSizeToRecv!=0 )
	{
	
		ret = recv(skt,lpRecvBuf  + iRecvOffset,iSizeToRecv - iRecvOffset,0);
		if (SOCKET_ERROR == ret && GetLastError() != WSAEWOULDBLOCK  || 0 == ret) // disconnected
		{
#ifdef _DEBUG
			std::cout << "Receive Error:" << GetLastError();
#endif
			DWORD err = GetLastError();
			throw new SocketException(err);
			
		}
		if (ret >0)
		{
			iRecvOffset += ret;

			pStatistics.BytesRead += ret;
			pStatistics.BytesReadLast += ret;

			InterlockedExchangeAdd(&gStatistics.BytesRead,ret);
			InterlockedExchangeAdd(&gStatistics.BytesReadLast,ret);
		}
		
	}
	
	if (iRecvOffset == iSizeToRecv && ret != 0) //get all data
	{
		lpQueue->Push(lpRecvBuf,iSizeToRecv,this);
		iSizeToRecv = iRecvOffset = 0;
		WORD cmd ;
		memcpy(&cmd,lpRecvBuf,2);
		if (cmd == CMD_PUSH_DATA /*&& lpmCacheNode != NULL*/)
		{
			lpmCacheNode->dwRecvCount++;
		}
		pStatistics.Calculate();
		gStatistics.Calculate();
	}
	return ret;
}