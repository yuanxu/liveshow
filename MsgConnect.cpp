#include "StdAfx.h"
#include "MsgConnect.h"
#include "stream.h"

MsgConnect::MsgConnect(void) : ReliableMessageBase()
{
	wCMD = CMD_CONNECT;
	bIsControl = true;
}

MsgConnect::~MsgConnect(void)
{
}

void MsgConnect::fromBytes(const char* p,const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwSID = lpReader->readDWORD();

	delete lpReader;
}

void MsgConnect::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE+4);
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE+4);
	lpWriter->writeDWORD(dwChID);
	
	lpWriter->writeDWORD(dwSID);
	
	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;
}

//-----------------------------------------------------------------------------

MsgConnectR::MsgConnectR() : MsgExBM()
{
	wCMD = CMD_CONNECT_R;
}

MsgConnectR::~MsgConnectR()
{
}