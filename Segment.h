/*

保存1秒钟数据

*/

#pragma once
#include "common.h"
#include "sys.h"

using namespace std;

enum PushDataResult
{
	//压入成功，且具备全部数据
	PushData_OK,
	//压入成功，但数据并不完整
	PushData_Partail,
	//重复数据
	PushData_NotUse,
	//段已过期
	PushData_OutOfDeadLine
};

/************************************************************************/
/* 保存1秒钟数据                                                        */
/************************************************************************/
class Segment
{
public:
	Segment(void);
	~Segment(void);
private:
	SEGMENTID			 m_segID;					//本segment的ID
	char				 m_DeadLine; 
	
	bool				 m_hasData;					//是否已经有数据
	
	
	
	static int			m_SegmentSize;				//Segment大小，单位字节
	static int			m_PacketsOfSegment ;		//每个segment包含的Packet数量
	static int			m_ChunkSize;				//asf Chunk的大小
public:
	vector<bool>		 m_PM;						//Segment中各个packet的标记位
	char*				 m_Data;					//数据体
//	void (*setBMFunction)(WORD segID);				//函数指针。Buffer需要实现此函数，并将函数指针赋值给本变量
	WLock*				m_Lock	;
	vector<DWORD>		vtSuppliers;				//数据提供者的SID列表
	time_t				t_ApplyTime;				//初次申请时间
	bool				bDelivered;					//数据是否已经被下发下去
public:
	void Reset(void)								//重置Segment
	{
		Reset(0);
	}
	void Reset(SEGMENTID id)
	{
		Reset(id,MAX_LIVE_TIME);
	}

	void Reset(SEGMENTID id,byte deadline);

	inline int getPacketCount()
	{
		//return static_cast<int>(std::count_if(m_PM.begin(),m_PM.end(),true));
		int cnt = 0;
		for(vector<bool>::iterator it = m_PM.begin();it != m_PM.end();it++)
		{
			if((*it))
				cnt++;
		}
		return cnt;
	};

	inline SEGMENTID getSegID()	{ return m_segID; };

	//将接收缓冲中的数据存入segment的指定packet中
	
	PushDataResult PushData(byte seq,const char *p,int Length=Segment::getRealPacketSize());		

	
	inline bool getData(char** p)
	{
		if( !m_hasData) 
			return false;
		if ( NULL == *p)
			*p = new char[Segment::getSegmentSize()];
		else if(sizeof(*p)< Segment::getSegmentSize())
		{
			delete[] *p;
			*p = new char[Segment::getSegmentSize()];
		}
		memcpy(*p,m_Data,Segment::getSegmentSize());

		return true;
	};

	//没有完全数据时只也返回得到的正确数据
	int getData2(char **p,bool bIsImmediate)
	{
		/*if (getData(p))
		{
			return Segment::getSegmentSize();
		}
		else
		{
			return 0;
		}*/
		m_Lock->on();
		if (m_hasData && bIsImmediate)
		{
			getData(p);
			m_Lock->off();
			return Segment::getSegmentSize();
		}
		else if (m_hasData)
		{
			getData(p);
			m_Lock->off();
			return Segment::getSegmentSize();
		}
		else
		{

			int iCnt =0;
			int iSize = 0;
			for (vector<bool>::iterator it = m_PM.begin();it!= m_PM.end();it++)
			{
				if(*it)
					iCnt++;
			}
			if (iCnt != 0)
			{
				if (NULL==*p)
				{
					*p = new char[Segment::getRealPacketSize()*iCnt];
				}
				iCnt =0;
				int iFilledCnt =0;
				
				for (vector<bool>::iterator it = m_PM.begin();it!= m_PM.end();it++)
				{
					if ((*it))
					{
						if (iCnt!= getChunksOfPackage() -1) //not last
						{
							memcpy(*p+Segment::getRealPacketSize()*iFilledCnt,m_Data+iCnt*getRealPacketSize(),getRealPacketSize());
							iSize+=getRealPacketSize();
						}
						else
						{
							memcpy(*p+Segment::getRealPacketSize()*iFilledCnt,m_Data+iCnt*getRealPacketSize(),getLastPacketSize());
							iSize+=getLastPacketSize();
						}
						
						iFilledCnt++;
					}
					iCnt++;
				}
		
			}
			m_Lock->off();
			return iSize;
		}
	}

	inline bool getHasData(void){return m_hasData;}

	inline vector<bool> GetPacketMap(void){return m_PM;} 

	inline void DecreasetDeadLine()	{ m_DeadLine--;}

	inline static void setSegmentSize(int size)
	{
		Segment::m_SegmentSize = size;
		//Segment::m_PacketsOfSegment = (size % PACKET_DATA_SIZE) == 0
		//								? size / PACKET_DATA_SIZE
		//								: size / PACKET_DATA_SIZE +1;

	}
	inline static void setChunkSize(int size)
	{
		m_ChunkSize = size;
	}

	inline static int getChunksOfPackage()
	{
		return PACKET_DATA_SIZE / m_ChunkSize;
	}
	
	inline static int getPacketsOfSegment(void)
	{
		Segment::m_PacketsOfSegment = (	m_SegmentSize % getRealPacketSize()) == 0
										? m_SegmentSize /  getRealPacketSize()
										: m_SegmentSize / getRealPacketSize() +1;
		return Segment::m_PacketsOfSegment;
	}

	inline static int getSegmentSize(void)
	{
		return Segment::m_SegmentSize;
	}
	inline static int getRealPacketSize()
	{
		return getChunksOfPackage() * m_ChunkSize ;
	}
	//最有一个包大小
	inline static int getLastPacketSize(void)
	{
		return Segment::m_SegmentSize % getRealPacketSize()  == 0
										?  getRealPacketSize()
										: Segment::m_SegmentSize  - ((Segment::getPacketsOfSegment() -1) *getRealPacketSize() ) ;
	}

	inline int getDeadline(){return m_DeadLine; };


};


//Segment 链
class SegmentList
{
public:
	SegmentList();
	~SegmentList();
private:
	//链表结构定义
	struct SegmentNode
	{
		Segment*		Segment;
		SegmentNode		*Next;
	};
	
	SegmentNode			*m_lpRoot, //链表根节点
						*m_lpTail; //最后一个节点
	WLock				*m_Lock;
	//deadline -= deadline
	static SEGMENTID dwSegID_Start;//辅助，为了下边的函数的特殊用途

	//辅助函数，在循环中被调用
	static bool ResetListProc(Segment *p)
	{
		p->Reset(dwSegID_Start);
		dwSegID_Start ++;
		return true;
	};
public:
	//逐个处理链表中的每个Segment。要求传递一个函数指针过来，作为处理函数，函数中要求具有 bool fun(Segment *)签名。
	//在本函数中会利用临界区提供线程安全机制
	void	ProcessSegment_s(bool (*Processor)(Segment*));
	//非线程安全版本
	void	ProcessSegment(bool (*Processor)(Segment*));
	//创建链表
	void	CreateList(SEGMENTID startSegID);
	//时间步进。将所有Segment的Deadline-1，并调整链表结构，取出deadline<0的节点
	void	TimeStep();
	void	ResetList(SEGMENTID startSegID)
	{
		dwSegID_Start = startSegID;
		ProcessSegment(SegmentList::ResetListProc);
	};
	PushDataResult PushData(SEGMENTID segID,byte seq,const char *p,int Length=Segment::getRealPacketSize());
	Segment* getSegment(SEGMENTID segID);
};