#include "StdAfx.h"
#include "Buffer.h"
#include "stream.h"

//--------------------------------------------------------------------------------
Buffer::Buffer(void)
	:m_TimerElapse(TIMER_PRELUDE)
{
	m_lpTimer = NULL;
	m_lpBM = new BufferMap();
	m_lpSegList = new SegmentList();
	m_Header.lpData = NULL;
	m_Header.len = 0;
	m_TimerElapse = 0;
	wServerCurrSegID = 0;
//	m_lpDeliver = 0;
}


Buffer::~Buffer(void)
{
	timeKillEvent(m_lpTimer);
	delete m_lpBM;
	delete m_lpSegList;
}



	//定时期处理程序
/*void CALLBACK Buffer::TimerProc(HWND hwnd,
				UINT uMsg,
				UINT_PTR idEvent,
				DWORD dwTime
		)*/
void CALLBACK Buffer::TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{

	Buffer *p = (Buffer *)dwUser;
	if(p->bIsSrv)
		return;
	if(TIMER_PRELUDE == p->m_TimerElapse) //第一次时间间隔到达
	{
		//重新设置timer为每秒钟触发
		//KillTimer(NULL,p->m_lpTimer);
		//timeKillEvent(p->m_lpTimer/*uTimerID*/);
		p->startNormalTimer();
	
	}
	//检查是否符合TimeStep条件
	//if(p->m_lpBM->getFullRate() > 50) //<10?
		p->TimeStep();
	
	/*for(int i =0;i<20 ;i++)
	{
		p->TimeStep();
	}*/
};

void Buffer::setTimer()
{
	return;
	if (m_TimerElapse == TIMER_PRELUDE)
	{
		return;
	}
	clearTimer();	//清除之前设置的定时器
	m_lpTimer =  timeSetEvent(TIMER_PRELUDE,50,TimerProc,(DWORD_PTR)this,TIME_ONESHOT);
	m_TimerElapse =TIMER_PRELUDE;

};

