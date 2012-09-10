#include "stdafx.h"
#include "protocol.h"
#include "stream.h"
#define VERSION 0x0001
/*
using namespace Protocol;

CMD Protocol::getConnect(DWORD sid,const char *ChID)
{
	MemoryStream *lpWriter = new MemoryStream(1024);
	
	WORD len =0,seq = getSeq();
	lpWriter->writeShort(CMD_CONNECT);		//cmd
	lpWriter->writeShort(seq);				//seq
	lpWriter->writeShort(len);				//len

	lpWriter->writeInt(sid);				//sid
	lpWriter->writeShort(VERSION);			//ver
	lpWriter->writeString(ChID);			//chid

	memcpy(lpWriter->buf+4,&len,2);			//len

	CMD cmd ;
	
	cmd.buf = new char[ lpWriter->len];
	cmd.buflen = lpWriter->len;

	memcpy(cmd.buf,lpWriter->buf,cmd.buflen);
	cmd.wCmd = CMD_CONNECT;
	cmd.wSeq = seq;
	delete lpWriter;
	return cmd;
};

CMD Protocol::getConnectR(WORD wSeq,ConnectResult Result,DWORD ip,WORD port)
{
	CMD cmd ;
	cmd.wCmd = CMD_CONNECT_R;
	cmd.wSeq = wSeq;
	
	MemoryStream *lpWriter = new MemoryStream(512);
	lpWriter->writeShort(cmd.wCmd);
	lpWriter->writeShort(cmd.wSeq );
//	lpWriter->writeShort(cmd.buflen);
	

	switch(Result)
	{
	case Connect_Result_OK :
		lpWriter->writeChar('\x0');
		break;
	case Content_Result_ReDirection:
		lpWriter->writeChar('\x1');
		lpWriter->writeInt(ip);
		lpWriter->writeShort(port);
		
		break;
	case Content_Result_Require_New_Version:
		lpWriter->writeChar('\x2');
		lpWriter->writeShort(VERSION);
		break;
	}
	int len = lpWriter->len;
	cmd.buf = lpWriter->buf;
	memcpy(cmd.buf + 4,&len,2);
	
	cmd.buflen = len;
	
	delete lpWriter;
	return cmd;
};
*/