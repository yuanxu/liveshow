#pragma once
/************************************************************************/
/*数据源接口，只有在Server状态可用                                      */
/************************************************************************/
#include "Buffer.h"
#include "sys.h"

class SourcePin
{
public:
	SourcePin(void);
	~SourcePin(void);
	void Start(Buffer* lpBuf);
	void Stop();

private:
	bool bIsRunning ;
	static int DataProcessFunc(ThreadInfo *lpInfo);
	ThreadInfo *lpThrd;
	Buffer *lpBuf;
};
