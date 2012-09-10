#include "StdAfx.h"
#include "UdpMessages.h"
#include "stream.h"

void UdpHello::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)p,len);

	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	SID = lpReader->readDWORD();
	delete lpReader;
}

void UdpHello::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE+4);
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE);
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(SID);
	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;
}


//////////////////////////////////////////////////////////////////////////

void UdpHelloR::fromBytes(const char*p,const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)p,len);

	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	if (bufLen!=len)
	{
		return;
	}
	SID = lpReader->readDWORD();

	int offset = PTL_HEADER_SIZE;
	while ((len - offset) >= SizeOfUdpCmd)
	{
		UdpCmd *cmd = new UdpCmd();
		cmd->wCMD = lpReader->readWORD();
		cmd->dwSID = lpReader->readDWORD();
		cmd->dwIP = lpReader->readDWORD();
		cmd->wPort = lpReader->readWORD();
		cmd->CHID = lpReader->readDWORD();
		vtCmds.push_back(cmd);
		offset += SizeOfUdpCmd;
	}

	delete lpReader;
}

void UdpHelloR::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE + SizeOfUdpCmd * static_cast<int>(vtCmds.size()) + 4);
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);

	lpWriter->writeWORD(PTL_HEADER_SIZE + SizeOfUdpCmd * static_cast<int>(vtCmds.size()) + 4);
	lpWriter->writeDWORD(dwChID);
	
	lpWriter->writeDWORD(SID);

	int offset = PTL_HEADER_SIZE;
	for (vector<UdpCmd*>::iterator it = vtCmds.begin(); it != vtCmds.end() ; it++)
	{
		lpWriter->writeWORD((*it)->wCMD);
		lpWriter->writeDWORD((*it)->dwSID);
		lpWriter->writeDWORD((*it)->dwIP);
		lpWriter->writeWORD((*it)->wPort);
		lpWriter->writeDWORD((*it)->CHID);
	}

	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;
}

//////////////////////////////////////////////////////////////////////////

void UdpCallBack::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)p,len);

	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	SID = lpReader->readDWORD();

	Body.wCMD = lpReader->readWORD();
	Body.dwSID = lpReader->readDWORD();
	Body.dwIP = lpReader->readDWORD();
	Body.wPort = lpReader->readWORD();
	Body.CHID = lpReader->readDWORD();

	delete lpReader;
}

void UdpCallBack::toBytes()
{

	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE + 16 +4);
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE + 16+4);
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(SID);

	//memcpy(lpWriter->buf+PTL_HEADER_SIZE,&Body,sizeof(UdpCmd));
	lpWriter->writeWORD(Body.wCMD);
	lpWriter->writeDWORD(Body.dwSID);
	lpWriter->writeDWORD(Body.dwIP);
	lpWriter->writeWORD(Body.wPort);
	lpWriter->writeDWORD(Body.CHID);

	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;

}