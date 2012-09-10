#pragma once
#include "Segment.h"
#include "sys.h"
#include <map>
#include "BufferMap.h"
#include "IDeliver.h"

using namespace std;

#define TIME_STEP_TIMER	1*1000	//TimeStep例程的执行间隔
#define TIMER_PRELUDE  (UINT)(MAX_SEGMENT_COUNT *0.8 * 1000) // 第一次启动Timer的时间间隔，单位毫秒。
#define DELIVER_PROC	void (*Proc)(Segment*,bool) //数据下发程序签名
#define DELIVER_PROC_TIMER	667	//Deliver例程的执行间隔每两秒执行三次
class Buffer
{
public:
	Buffer(void);
	~Buffer(void);
	
private:
	
	SegmentList*	m_lpSegList;
	
	//设置定时器
	
	static void CALLBACK TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	
	vector<IDeliver*> vtDeliverProcs;
public:
	

	struct _MediaHeader_TAG
	{
		char* lpData ;
		int len;
		SEGMENTID dwStartID;
	} m_Header;

	BufferMap*		m_lpBM;	

	UINT			m_lpTimer;
	int				m_TimerElapse;//时间间隔


	SEGMENTID			wServerCurrSegID;//主服务器上当前的SegmentID


	//设定初始定时器。此定时器仅执行一次，之后被正常定时器Normal取代
	void			setTimer();
	bool			bIsSrv;//是否是服务器端

	void			clearTimer()
	{
		if(NULL != m_lpTimer)
			timeKillEvent( m_lpTimer);
		m_lpTimer = NULL;
	};

	//设置正常执行的定时器，此定时器每秒执行一次
	void		startNormalTimer()
	{
		return;
		if(m_TimerElapse == TIME_STEP_TIMER)
			return;
		clearTimer();

		m_lpTimer = timeSetEvent(TIME_STEP_TIMER,50,TimerProc,(DWORD_PTR)this,TIME_PERIODIC);
		m_TimerElapse = TIME_STEP_TIMER;

	}
	
	///服务器端专用。不启动定时器
	PushDataResult srv_PushData(SEGMENTID segID,byte seq,const char *p,int Length=Segment::getRealPacketSize())	
	{
		if( NULL == m_lpTimer)
		{
			m_lpTimer = 1;
			m_TimerElapse = NULL;
		}
		PushDataResult ret = PushData(segID,seq,p,Length) ;
		if(PushData_OutOfDeadLine == ret)
		{
			TimeStep();
			return PushData(segID,seq,p,Length);
		}
		else
		{
			return ret;
		}
	}
	
	PushDataResult srv_PushData(SEGMENTID segID,const char *p,int Length)	
	{
		for (char cSeq = 0;cSeq < Segment::getPacketsOfSegment();cSeq++)
		{
			if (cSeq==Segment::getPacketsOfSegment() -1) //last packet
			{
				PushData(segID,cSeq,p+Segment::getRealPacketSize()*cSeq,Segment::getLastPacketSize());

			}
			else
			{
				PushDataResult ret  = PushData(segID,cSeq,p+Segment::getRealPacketSize()*cSeq);
				if(PushData_OutOfDeadLine == ret)
				{
					TimeStep();
					PushData(segID,cSeq,p+Segment::getRealPacketSize()*cSeq);
				}
			}
		}
		return PushData_OK;
		
	}
	//客户端专用。必要时启动定时器
	PushDataResult PushData(SEGMENTID segID,byte seq,const char *p,int Length=Segment::getRealPacketSize())
	{
		m_lpBM->m_Lock->on();

		PushDataResult rst = m_lpSegList->PushData(segID,seq,p,Length);


		if(PushData_OK == rst) //具备全部数据后，设置bm
		{
#if _DEBUG
		//char buf[512];
			//sprintf(buf,"Get Data .segID:%d",segID,seq);
			//log::WriteLine(buf );
			
#endif
			m_lpBM->SetBM(segID);

		if((NULL == m_lpTimer || m_TimerElapse == TIMER_PRELUDE) && (this->m_lpBM->getFullRate() >= 80 || segID > m_lpBM->getStartID() * MAX_SEGMENT_COUNT* 0.8)  ) //TODO:服务器端不启动定时器
		{
			//TODO:如果本机的StartID与服务器StartID超过25%，则不启动
			if(abs(m_lpBM->getStartID() - wServerCurrSegID) > MAX_SEGMENT_COUNT * STARTID_MARGIN)
			{
				startNormalTimer();
			}
		}

		}
		else if(PushData_Partail == rst)
		{
		
		}
		else
		{
#if _DEBUG
			//sprintf(buf,"Segment%d is Out of deadline.",segID);
			//log::WriteLine(buf);
#endif
		}
		
		m_lpBM->m_Lock->off();
		if (rst==PushData_OK)
		{
			Deliver(getSegment(segID),true);
		}
		return rst;
	}

	//调整Buffer。每次调用调整一秒钟的数据
	void TimeStep()
	{
		if(m_lpTimer == 1) //作为服务器运行
		{
			//Buffer::DeliverProc(0, 0, (DWORD_PTR)this, (DWORD_PTR)getStartID(),NULL);
		}
		m_lpSegList->TimeStep();
		m_lpBM->TimeStep();
	}

	//设置当前段。定时与服务器同步时会调用此方法
	void SetPosition(SEGMENTID segID)
	{

		
		while(segID > getStartID()) 
		{
			Deliver(getSegment(getStartID()),false);
			TimeStep();//调整

		}
	
	}

	void Reset(SEGMENTID segID)
	{
		m_lpSegList->ResetList(segID);
		m_lpBM->Reset(segID);
	}

	void Init(SEGMENTID segID)
	{
	
		m_lpSegList->CreateList(segID);
	
		m_lpBM->setStartID( segID);
	}
	Segment* getSegment(SEGMENTID segID)
	{
		return m_lpSegList->getSegment(segID);
	}

	inline SEGMENTID getStartID()
	{
		return m_lpBM->getStartID();
		
	}


	/************************************************************************/
	/* 注册Deliver函数　                                                     */
	/************************************************************************/
	void RegisterDeliver(IDeliver *lpProc)
	{
		lpProc->bDeliveredHeader= false;
		vtDeliverProcs.push_back(lpProc);
	}
	void UnRegisterDeliver(IDeliver *lpProc)
	{
		//TODO:not implement
		return;

	}
	void Deliver(Segment* p,bool isImmediate)
	{
		for (vector<IDeliver*>::iterator it = vtDeliverProcs.begin(); it != vtDeliverProcs.end() ; it++)
		{
			if(!(*it)->bDeliveredHeader)
			{
				(*it)->DeliverHeader(m_Header.lpData,m_Header.len);
				(*it)->bDeliveredHeader = true;
			}
			if (isImmediate)
			{
				(*it)->ImmediateDeliver(p,true);
			}
			else
			{
				(*it)->Deliver(p,false);
			}
		}
	}
};

