#include "StdAfx.h"
#include "MsgJoin.h"
#include "stream.h"

using namespace std;
MsgJoin::~MsgJoin(void)
{
	
}


MsgJoin::MsgJoin(const char *lpBuf,const int bufLen):ReliableMessageBase()
{
	fromBytes(lpBuf,bufLen);
}
//从数组中恢复对象
void MsgJoin::fromBytes(const char *lpBuf,const int len)
{
	MemoryStream *lpReader = new MemoryStream((void *)lpBuf,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwSID = lpReader->readDWORD();
	wVersion = lpReader->readWORD();
	int iCnt = lpReader->readChar(); // amount of ips
	for (int i = 0 ; i < iCnt ; i++)
	{
		vtIP.push_back(lpReader->readDWORD());
	}
	
	wPort = lpReader->readWORD();

	//szChID = new char[bufLen - PTL_HEADER_SIZE + 7/*version(2) ,sid(4),ipcount(1)*/ + vtIP.size() *4];
	//lpReader->readString(szChID);
	
	delete lpReader;
}

//将数组转换成对象
void MsgJoin::toBytes(char **lpBuf, int *len)
{
	toBytes();
	lpBuf = lpBuf;
	*len = bufLen;
}

void MsgJoin::toBytes()
{
	WORD len = PTL_HEADER_SIZE + 9/*version(2) ,sid(4),ipcount(1),port*/ + vtIP.size()*4 ;
	MemoryStream *lpWriter = new MemoryStream(len);
	
	WORD seq = wSeqID;

	lpWriter->writeWORD(CMD_JOIN);			//cmd
	lpWriter->writeWORD(seq);				//seq
	lpWriter->writeWORD(len);				//len
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(dwSID);				//sid 4
	lpWriter->writeWORD(VERSION);			//ver 2
	lpWriter->writeChar(static_cast<char>(vtIP.size()));//amout of ip 1
	for (vector<DWORD>::iterator it = vtIP.begin() ; it != vtIP.end() ; it++)
	{
		lpWriter->writeDWORD(*it);
	}
	lpWriter->writeWORD(wPort); //2

	//lpWriter->writeString(szChID);			//chid
	

	
	len = bufLen = lpWriter->pos;

	memcpy(lpWriter->buf+4,&len,2);			//len
	
	lpBuf = lpWriter->buf;
	bufLen = lpWriter->pos;
	
//	delete[] (lpBuf+len);	//删除多余的数据
	wCMD = CMD_JOIN;
	
	delete lpWriter;
	
}

//-------------------------------------------------------------------------------

void MsgJoinR::fromBytes(const char *lpBuf, const int len)
{
	MemoryStream *lpReader = new MemoryStream((void*)lpBuf,len);
	wCMD = lpReader->readWORD();
	wSeqID = lpReader->readWORD();
	bufLen = lpReader->readWORD();
	dwChID = lpReader->readDWORD();

	dwServerSID = lpReader->readDWORD(); //服务器ID
	dwClientSID = lpReader->readDWORD();//客户机ID

	switch(lpReader->readChar())
	{
		case 0:
			Result = Join_Result_OK;
			break;
		case 1:
			Result = Join_Result_ReDirection ;
			dwIP = lpReader->readDWORD();
			wPort = lpReader->readWORD();
			break;
		case 2:
			Result = Join_Result_Require_New_Version;
			wVersion = lpReader->readWORD();
			break;
	}

	delete lpReader;
}

void MsgJoinR::toBytes()
{
	MemoryStream *lpWriter = new MemoryStream(PTL_HEADER_SIZE+15);
	lpWriter->writeWORD(wCMD);
	lpWriter->writeWORD(wSeqID);
	lpWriter->writeWORD(0); //占位
	lpWriter->writeDWORD(dwChID);

	lpWriter->writeDWORD(dwServerSID); //4
	lpWriter->writeDWORD(dwClientSID); //4

	switch(Result)
	{
	case Join_Result_OK:
		lpWriter->writeChar('\0');
		break;
	case Join_Result_ReDirection:
		lpWriter->writeChar('\1'); //1
		lpWriter->writeDWORD(dwIP); //4
		lpWriter->writeWORD(wPort);//2
		break;
	case Join_Result_Require_New_Version:
		lpWriter->writeChar('\2');
		lpWriter->writeWORD(VERSION);
		break;
	}
	lpBuf = lpWriter->buf;
	bufLen = lpWriter->pos;

	lpWriter->pos= 4;
	lpWriter->writeWORD(bufLen);
	delete lpWriter;
}