#include "StdAfx.h"
#include "MsgPushServant.h"
#include "stream.h"
#include <time.h>


MsgPushServant::MsgPushServant(void):ReliableMessageBase()
{
	wCMD = CMD_PUSH_SERVANTS;
}

MsgPushServant::~MsgPushServant(void)
{
}

void MsgPushServant::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	while(!lpReader->eof())
	{
		ServantInfo* info = new ServantInfo();
		info->dwSID = lpReader->readDWORD();
//		info.wSeqID = lpReader->readWORD();
		info->addr.sin_family = AF_INET;
		info->addr.sin_addr.s_addr = lpReader->readDWORD();
		info->addr.sin_port = lpReader->readWORD();
		memset(info->addr.sin_zero,'\0',sizeof(info->addr.sin_zero));

		vtSvts.push_back(info);
	}

	delete lpReader;
}

void MsgPushServant::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PACKET_SIZE);
	lpWriter->writeWORD(CMD_PUSH_SERVANTS);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(0);
	lpWriter->writeDWORD(dwChID);

	for(std::vector<ServantInfo*>::iterator it = vtSvts.begin() ; it != vtSvts.end() && !lpWriter->pos + 10 < lpWriter->len ; it++)
	{
		lpWriter->writeDWORD((*it)->dwSID);
//		lpWriter->writeWORD((*it).wSeqID);
		lpWriter->writeDWORD((*it)->addr.sin_addr.s_addr);
		lpWriter->writeWORD((*it)->addr.sin_port);
	}

	bufLen = lpWriter->pos;
	lpBuf = lpWriter->buf;
	lpWriter->pos = 4;
	lpWriter->writeWORD(bufLen);

	delete lpWriter;
}

////////////////////////////////////////////////////////////////////////////////

void MsgPushServantR::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)p,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();
	delete lpReader;

}

void MsgPushServantR::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE);
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(PTL_HEADER_SIZE);
	lpWriter->writeDWORD(dwChID);

	lpBuf = lpWriter->buf;
	
	bufLen = lpWriter->len;;
	delete lpWriter;
}