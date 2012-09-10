#include "StdAfx.h"
#include "MsgPushData.h"
#include "stream.h"
#include "common.h"

MsgPushData::MsgPushData(void):ReliableMessageBase()
{
	wCMD = CMD_PUSH_DATA;
	lpData = NULL;
	cSeq = 0;
	dataLen = -1;
}

MsgPushData::~MsgPushData(void)
{
	if(lpData)
		delete[] lpData;
}


void MsgPushData::fromBytes(const char* p,const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwSegID =lpReader->readDWORD();
	cSeq = lpReader->readChar();
	dataLen = len - PTL_HEADER_SIZE - sizeof(SEGMENTID)-1;
	if(NULL == lpData)
		lpData = new char[dataLen];
	lpReader->read(lpData,dataLen);
	
	delete lpReader;

}

void MsgPushData::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PACKET_SIZE);
	lpWriter->writeWORD(CMD_PUSH_DATA);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE+sizeof(SEGMENTID)+1+dataLen);
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(dwSegID);
	lpWriter->writeChar(cSeq);
	lpWriter->write(lpData,dataLen);

	lpBuf = lpWriter->buf;
	bufLen = PTL_HEADER_SIZE+sizeof(SEGMENTID)+1+dataLen;
	delete lpWriter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MsgPushDataR::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();
	
	delete lpReader;

}

void MsgPushDataR::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE);
	lpWriter->writeWORD(CMD_PUSH_DATA_R);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE);
	lpWriter->writeDWORD(dwChID);
	
	lpBuf = lpWriter->buf;
	bufLen = PTL_HEADER_SIZE;
	delete lpWriter;
}
