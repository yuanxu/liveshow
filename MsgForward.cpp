#include "StdAfx.h"
#include "MsgForward.h"
#include "Stream.h"

MsgForward::MsgForward(void)
{
}

MsgForward::~MsgForward(void)
{
	
}

void MsgForward::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = len;

	dwSID = lpReader->readDWORD();
	wVersion = lpReader->readWORD();
	dwIP = lpReader->readDWORD();
	wPort= lpReader->readWORD();
	lpReader->readString(szChID);

	delete lpReader;
}

void MsgForward::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(1024);
	lpWriter->writeWORD(CMD_FORWARD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(0);//	占位符

	lpWriter->writeDWORD(dwSID);
	lpWriter->writeWORD(wVersion);
	lpWriter->writeDWORD(dwIP);
	lpWriter->writeWORD(wPort);
	lpWriter->writeString(szChID);

	lpBuf = lpWriter->buf;;
	lpWriter->pos = 2;
	lpWriter->writeWORD(lpWriter->len);
	bufLen = lpWriter->len;

	delete lpWriter;

}

//-------------------------------------

void MsgForwardR::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = len;
	
	Result = lpReader->readChar();
	delete lpReader;
}

void MsgForwardR::toBytes()
{
		MemoryStream *lpWriter = new MemoryStream(7);
	lpWriter->writeWORD(CMD_FORWARD_R);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(7);//	占位符

	lpWriter->writeChar(Result);

	lpBuf = lpWriter->buf;;
	lpWriter->pos = 2;
	lpWriter->writeWORD(lpWriter->len);
	bufLen = lpWriter->len;

	delete lpWriter;

}