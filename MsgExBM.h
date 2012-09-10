#pragma once
#include "messagebase.h"

class MsgExBM :
	public MessageBase
{
public:
	MsgExBM(void)  : MessageBase()
	{
		wCMD = CMD_EXCHANGE_BM;
		bIsOwnerBuf = false;
		bIsControl = true;
	};
	~MsgExBM(void)
	{
		if(bIsOwnerBuf && NULL != lpBM)
		{
			delete[] lpBM;
		}
	};
private:
	bool bIsOwnerBuf;
public:
	DWORD dwSID;
	WORD wNumberOfPartners;
	WORD wBandWidth;
	NetworkType nwtType;
	DWORD dwRemoteBytesOfSent;
	DWORD dwRemoteBytesOfReceive;
	char cRevert ;
	char* lpBM;

public:
	void fromBytes(const char* p,const int len);
	void toBytes();
};

//---------------------------------------------------------------------------------------
/*
class MsgExBMR :
	public MessageBase
{
public:
	MsgExBMR(void) :MessageBase() 
	{
		wCMD = CMD_EXCHANGE_BM_R;
	};
	~MsgExBMR(void);
public:
	void fromBytes(const char *p,const int len);
	void toBytes();
};
*/