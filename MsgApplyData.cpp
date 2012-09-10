#include "StdAfx.h"
#include "MsgApplyData.h"
#include "stream.h"

void MsgApplyData::fromBytes(const char *p, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)p,len);

	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwSID = lpReader->readDWORD();

	while(!lpReader->eof())
	{
		vtSegIDs.push_back(lpReader->readDWORD());
	}
	delete lpReader;
}

void MsgApplyData::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE +sizeof(SEGMENTID)  + static_cast<int> (vtSegIDs.size()) * sizeof(SEGMENTID));
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(lpWriter->len);
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(dwSID);

	std::vector <SEGMENTID>::const_iterator c1_Iter;
	while(vtSegIDs.size() > 0)
	{
		//c1_Iter = vtSegIDs.back();
		SEGMENTID &id  = vtSegIDs.back();
		lpWriter->writeDWORD(id);
		vtSegIDs.pop_back();
	}

	lpBuf = lpWriter->buf;
	bufLen = lpWriter->len;

	delete lpWriter;
}

//-------------------------------------------------------------------------------
