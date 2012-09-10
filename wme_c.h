#pragma once
#include "Buffer.h"
#include "sys.h"

class wme_c
{
public:
	wme_c(Buffer *lpBuf){m_buf = lpBuf;};
	~wme_c(void);

private :
	Buffer * m_buf;
	ThreadInfo *lpThrd;
	char* getHeader(char*,WORD);
	void processData(char*,WORD); //数据处理程序
	void receive(char* ip,WORD port);
	int processData(Buffer *lpBuf, char* buf,int iPos,int iGet);
	
	//数据处理线程辅助代码
	struct IpInfo
	{
		wme_c *lpInstance;
		char* lpIP;
		WORD wPort;
	};
	IpInfo lpInfo ;
	static int DataProcessFunc(ThreadInfo *lpInfo);
	
	DWORD dwPacketSize;
	DWORD dwBitRate;
	//end of 辅助代码
	
public:
	void Start(char* ip, WORD port);
	void Stop();
	
	
};
