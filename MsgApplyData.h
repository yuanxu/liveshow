#pragma once
#include "messagebase.h"
#include <vector>

class MsgApplyData :
	public ReliableMessageBase
{
public:
	MsgApplyData(DWORD SID) : ReliableMessageBase()
	{
		dwSID = SID;
		wCMD = CMD_APPLY_DATAS;
		
	};
	
	~MsgApplyData(void) {vtSegIDs.clear();};

public:
	std::vector<SEGMENTID> vtSegIDs;
	DWORD dwSID;
public :
	void fromBytes(const char* p,const int len);
	void toBytes();
};


//-------------------------------------------------------------------------

class MsgApplyDataR :
	public MsgApplyData
{
public:
	MsgApplyDataR(DWORD SID):MsgApplyData(SID)
	{
		wCMD = CMD_APPLY_DATAS_R;
		bNeedConfirm =FALSE;
	};
	~MsgApplyDataR(void){};

};