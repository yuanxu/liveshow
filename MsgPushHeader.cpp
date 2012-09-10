#include "StdAfx.h"
#include "MsgPushHeader.h"
#include "Stream.h"

MsgPushHeader::MsgPushHeader(void):ReliableMessageBase()
{
	wCMD = CMD_PUSH_HEADER;
	lpData = NULL;
	bIsOwnerBuf = false;
}

MsgPushHeader::~MsgPushHeader(void)
{
	if(bIsOwnerBuf && lpData )
		delete[] lpData;
}

void MsgPushHeader::fromBytes(const char *p, const int len)
{
	bIsOwnerBuf = true;
	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();
	
	this->wInitSegID = lpReader->readDWORD(); //4

	dataLen = bufLen - PTL_HEADER_SIZE - sizeof(SEGMENTID);
	lpData = new char[dataLen];
	lpReader->read((void *)lpData,dataLen);


	delete lpReader;
}

void MsgPushHeader::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE + sizeof(SEGMENTID)+dataLen); //假定头的长度不超过8K

	lpWriter->writeWORD(CMD_PUSH_HEADER);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE + sizeof(SEGMENTID)+dataLen);	//
	lpWriter->writeDWORD(dwChID);
	
	lpWriter->writeDWORD(wInitSegID);
	lpWriter->write((void*)lpData,dataLen);

	lpBuf = lpWriter->buf;
	
	bufLen = lpWriter->len;
	
	delete lpWriter;
}

//------------------------------------------------------------------------------------------------

void MsgPushHeaderR::fromBytes(const char *p, const int len)
{

}

void MsgPushHeaderR::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE); //假定头的长度不超过8K
	lpWriter->writeWORD(CMD_PUSH_HEADER_R);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE);
	lpWriter->writeDWORD(dwChID);
	
	lpBuf = lpWriter->buf;
	
	bufLen = lpWriter->len;;
	delete lpWriter;

}