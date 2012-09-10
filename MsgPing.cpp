#include "StdAfx.h"
#include "MsgPing.h"
#include "stream.h"

MsgPing::MsgPing(void)
:MessageBase()
{
	wCMD = CMD_PING;
	bIsControl = true;
}

MsgPing::~MsgPing(void)
{
}

void MsgPing::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)p,len);

	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwSID = lpReader->readDWORD();
	dwIP = lpReader->readDWORD();
	wPort = lpReader->readWORD();
	ttl = lpReader->readChar();
	hops = lpReader->readChar();
	senderSID = lpReader->readDWORD();
	
	delete lpReader;
}

void MsgPing::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE+16);
	lpWriter->writeWORD(CMD_PING);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE+16);
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(dwSID); //4
	lpWriter->writeDWORD(dwIP); //4
	lpWriter->writeWORD(wPort); //2
	lpWriter->writeChar(ttl); //1
	lpWriter->writeChar(hops); //1
	lpWriter->writeDWORD(senderSID); //4

	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;
}
