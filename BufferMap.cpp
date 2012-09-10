#include "StdAfx.h"
#include "BufferMap.h"
#include "stream.h"

BufferMap::BufferMap(void)
	:m_StartID(0)
{
	m_Lock = new WLock();
	m_bts = new char[BUFSIZ];
	
	memset(m_bts,0,BUFSIZ);
}

BufferMap::BufferMap(SEGMENTID segID)
	:m_StartID(segID)
{
	m_Lock = new WLock();
	m_bts = new char[BUFSIZ];
	memset(m_bts,0,BUFSIZ);
}

void BufferMap::fromBytes(const char *p,int offset )
{
	if(offset>32)
		throw -1;

	m_Lock->on();
	MemoryStream *stream = new MemoryStream((void *)p,BM_SIZE);
	
	m_StartID = stream->readDWORD()+offset;

	//m_Bits.reset();
	for(int i =0;i < BM_SIZE - sizeof(SEGMENTID) ; i++ )
	{
		bitset<8> bs_t(stream->readChar());
		int iStart =0,iMax=8;
		if(i == 0)
		{
			iStart=offset;
		//	iMax = 32-offset;
		}
		else
		{
			iStart= 0;
			iMax=8;
		}
		for(int j = iStart; j < iMax  && ( i * 8 + j < static_cast<int>(m_Bits.size())) ;j++)
		{
			m_Bits[i*8 +j ] = bs_t[j];
		}
	}
	
	if( NULL == m_bts )
	{
		m_bts = new char[BM_SIZE];
	}
	memset(m_bts,0,BM_SIZE);
	memcpy(m_bts,p,BM_SIZE);
	delete stream;

	m_Lock->off();
}
BufferMap::~BufferMap(void)
{
	delete m_bts;
	delete m_Lock;
}

//调整BuferMap，向前移动一秒
void BufferMap::TimeStep()
{
	
	m_Lock->on();
	m_StartID++; //向前移动一秒
	
#if _DEBUG
	if(m_Bits[0])
		std::cout << endl << "StartID:" << m_StartID << endl;
	else
		std::cout << endl << "StartID:" << m_StartID << " But has NO DATA!!!" << endl;
#endif
	for(int i = 0; i < MAX_SEGMENT_COUNT - 1;i++)
	{
		m_Bits[i] = m_Bits[i+1];
	}
	m_Bits.reset(MAX_SEGMENT_COUNT -1 );
	toBytes_i();	//缓冲到字节数组
	m_Lock->off();
}

//转成char*
void BufferMap::toBytes_i(void)
{
	MemoryStream *stream = new MemoryStream(BM_SIZE);
	int iT = BM_SIZE;
	stream->writeDWORD(m_StartID);
	bitset<8> bs_t;
	for(int i =0;i < MAX_SEGMENT_COUNT ;i+=8)
	{
		bs_t.reset();
		for(int j=0;j<8;j++)
		{
			bs_t[j] = m_Bits[i];
		}
		char v = static_cast<char>(bs_t.to_ulong());
		stream->writeChar(v);
		
	}
	delete[] m_bts;
	m_bts = stream->buf;
	delete stream;
}

char* BufferMap::toBytes()
{
	return m_bts;
}


void BufferMap::checkData(std::vector<SEGMENTID> &vt)
{
	vector<SEGMENTID>::iterator it = vt.begin();
	while(it!=vt.end())
	{
		int iLoc =  (*it) - m_StartID;
		if(iLoc <0 || (iLoc < static_cast<int>(m_Bits.size()) -1 && !m_Bits[iLoc])) //不存在，则删除
		{
			it = vt.erase(it);
			continue;
		}
		it++;
	}
}