#pragma once
#include <bitset>
#include "common.h"
#include "sys.h"
#include "stream.h"

using namespace std;
class BufferMap
{
public:
	BufferMap(void);
	BufferMap(SEGMENTID segID);
	BufferMap(const char* p){fromBytes(p);};
	~BufferMap(void);
private:
	SEGMENTID			 m_StartID;
	
	//转成byte数组，内部使用
	void			toBytes_i(void);
public:
	char*			m_bts;	//字节数组缓存

	SEGMENTID			getStartID(void) {return m_StartID;}
	void				setStartID(SEGMENTID segID){ m_StartID = segID ;}
	bitset<MAX_SEGMENT_COUNT> m_Bits;
	//void			setStartID(WORD segID) {m_StartID = segID;}

	char*			toBytes();
	WLock*			m_Lock;
	void			SetBM(SEGMENTID segID)
	{
		assert(segID - m_StartID < MAX_SEGMENT_COUNT  && segID - m_StartID >= 0);
		m_Lock->on();
		int cnt = m_Bits.count();
		m_Bits.set(segID - m_StartID);
		assert (m_Bits.count()>cnt);
		m_Lock->off();
	};
	void fromBytes(const char *p,int offset=0);

	//检查vt中的数据是否包含在BM中。
	void checkData(vector<SEGMENTID> &vt);

	bool checkData(SEGMENTID dwSegID)
	{
		if (dwSegID < m_StartID || (dwSegID - m_StartID) >= static_cast<int>(m_Bits.size())) return false;
		return m_Bits.at(dwSegID - m_StartID);
	}

	//时间脉冲函数。需要增加m_StartID，以及移位m_Bits;本类不提供定时器线程，需要调度时，需要从Buffer类调度
	//本函数线程安全。
	void			TimeStep();
	void Reset(SEGMENTID segID)
	{
		m_StartID = segID;
		m_Bits.reset();
		if(NULL != m_bts)
			delete[] m_bts;
		m_bts = NULL;
	}

	int getEmptyRate()
	{
		return 100 - getFullRate();
	};
	int getFullRate()
	{
		float Cnt = static_cast<float>(m_Bits.count());

		return Cnt /MAX_SEGMENT_COUNT *100 +0.5;
	};
};