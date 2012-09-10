#pragma once
#include "messagebase.h"
#include <vector>
using namespace std;

class MsgJoin :
	public ReliableMessageBase
{
	//属性

public:
	vector<DWORD> vtIP;	//本机监听IP
	WORD wPort;				//本机监听Port
	WORD wVersion;			//版本
	PEERID dwSID;			//SID
	
	//方法
public:
	MsgJoin(void):wPort(0),wVersion(0),dwSID(0),ReliableMessageBase()
	{};
	MsgJoin(const PEERID dwSID,const DWORD ChID):ReliableMessageBase()
	{
		this->dwSID = dwSID;
		dwChID = ChID;
		
		this->wVersion = 0;
		
	};
	MsgJoin(const char* lpBuf,const int bufLen);
	~MsgJoin(void);
	
	void fromBytes(const char *lpBuf,const int len);
	void toBytes();
	void toBytes(char** lpBuf,int *len);
};

//-------------------------------------------------------------------------------

enum JoinResult
{
	Join_Result_OK ,
	Join_Result_ReDirection ,
	Join_Result_Require_New_Version,
	Join_Result_ChID_Not_Match
};

//-------------------------------------------------------------------------------

class MsgJoinR:
	public ACKMessageBase
{
public:
	JoinResult Result;
	WORD wVersion;
	DWORD dwIP;
	WORD wPort;
	PEERID dwServerSID,dwClientSID;
public:
	MsgJoinR(char *lpBuf,int len):ACKMessageBase()
	{
		dwClientSID =0;
		dwServerSID =0;
		fromBytes(lpBuf,len);
		wCMD = CMD_JOIN_R;
		
	};
	MsgJoinR():ACKMessageBase()
	{
		wCMD = CMD_JOIN_R;
		dwClientSID =0;
		dwServerSID =0;
	};
	void fromBytes(const char *lpBuf,const int len =-1);
	void toBytes();

};