#include "StdAfx.h"
#include "MsgLeave.h"
#include "stream.h"

MsgLeave::MsgLeave(void):MessageBase()
{
}

MsgLeave::~MsgLeave(void)
{
}

void MsgLeave::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();
	
	delete lpReader;
}

void MsgLeave::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE);
	lpWriter->writeWORD(CMD_LEAVE_R);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE);
	lpWriter->writeDWORD(dwChID);

	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;
}