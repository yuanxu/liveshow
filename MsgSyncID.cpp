#include "StdAfx.h"
#include "MsgSyncID.h"
#include "stream.h"

MsgSyncID::MsgSyncID(void):ReliableMessageBase()
{
	wCMD = CMD_SYNC_ID;
}

MsgSyncID::~MsgSyncID(void)
{
}

void MsgSyncID::fromBytes(const char*p,const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwSID= lpReader->readDWORD();
	dwChID = lpReader->readDWORD();
	delete lpReader;
}

void MsgSyncID::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE+4);
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE+4);
	lpWriter->writeDWORD(dwSID);
	lpWriter->writeDWORD(dwChID);

	lpBuf = lpWriter->buf;
	bufLen = PTL_HEADER_SIZE;
	delete lpWriter;
}

//-------------------------------------
MsgSyncIDR::MsgSyncIDR():ACKMessageBase()
{
	wCMD = CMD_SYNC_ID_R;
}
MsgSyncIDR::~MsgSyncIDR()
{
}

void MsgSyncIDR::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwSegID = lpReader->readDWORD();
	delete lpReader;
}
void MsgSyncIDR::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE+sizeof(SEGMENTID));
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE+sizeof(SEGMENTID));
	lpWriter->writeDWORD(dwChID);


	lpWriter->writeDWORD(dwSegID);

	lpBuf = lpWriter->buf;
	bufLen = PTL_HEADER_SIZE+sizeof(SEGMENTID);
	delete lpWriter;
}