#include "StdAfx.h"
#include "MsgApplyPacket.h"
#include "stream.h"

MsgApplyPacket::MsgApplyPacket(void):MessageBase()
{
	wCMD = CMD_APPLY_PACKET;
	bIsControl = true;
}

MsgApplyPacket::~MsgApplyPacket(void)
{
	
}

void MsgApplyPacket::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwSegID = lpReader->readDWORD();
	cSeq = lpReader->readChar();

	delete lpReader;
}

void MsgApplyPacket::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE+sizeof(SEGMENTID)+1);
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE+sizeof(SEGMENTID)+1);
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(dwSegID);
	lpWriter->writeChar(cSeq);

	lpBuf = lpWriter->buf;
	bufLen = PTL_HEADER_SIZE+sizeof(SEGMENTID)+1;
	delete lpWriter;
}