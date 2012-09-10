#include "StdAfx.h"
#include "MsgExBM.h"
#include "stream.h"


void MsgExBM::fromBytes(const char *p,const int len)
{
	bIsOwnerBuf = true;

	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwSID = lpReader->readDWORD();
	wNumberOfPartners = lpReader->readWORD();
	wBandWidth = lpReader->readWORD();
	switch(lpReader->readChar())
	{
	case '\0':
		nwtType = NetType_Public;
		break;
	case '\1':
		nwtType = NetType_Private;
		break;
	}
	dwRemoteBytesOfSent = lpReader->readDWORD();
	dwRemoteBytesOfReceive = lpReader->readDWORD();
	lpBM = new char[BM_SIZE];
	lpReader->read(lpBM,BM_SIZE);
	cRevert = lpReader->readChar();

	delete lpReader;

}

void MsgExBM::toBytes()
{
	bIsOwnerBuf = false;

	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE + BM_SIZE +18);//
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE + BM_SIZE +18);
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(dwSID);//4
	lpWriter->writeWORD(wNumberOfPartners);//2
	lpWriter->writeWORD(wBandWidth);//2
	switch(nwtType)//1
	{
	case NetType_Public:
		lpWriter->writeChar('\0');
		break;
	case NetType_Private:
		lpWriter->writeChar('\1');
		break;
	default:
		lpWriter->writeChar('\0');
	}
	lpWriter->writeDWORD(dwRemoteBytesOfSent);//4
	lpWriter->writeDWORD(dwRemoteBytesOfReceive);//4
	lpWriter->write(lpBM,BM_SIZE);
	lpWriter->writeChar(cRevert);//1
	
	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;
}

//---------------------------------------------------------------------------------------
/*
void MsgExBMR::fromBytes(const char *p, const int len)
{
	//无须实现。因为没有包体
}

void MsgExBMR::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(6);
	lpWriter->writeWORD(CMD_EXCHANGE_BM_R);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(6);
	
	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;
}
*/