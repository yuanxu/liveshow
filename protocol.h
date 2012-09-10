#pragma once
#include "sys.h"
/*

#define CMD_UNKNOWN				0x0000	//未知
#define CMD_CONNECT				0x0001	//连接指令
#define CMD_FORWARD				0x0002	//前转指令
#define CMD_PUSH_HEADER			0x0003	//推送头
#define CMD_PUSH_SERVANTS		0x0004	//推送Servants
#define	CMD_LEAVE				0x0005	//离开
#define	CMD_ACTIVE				0x0006	//激活
#define CMD_APPLY_DATAS			0x0007	//请求数据
#define	CMD_PUSH_DATA			0x0008	//推送数据
#define CMD_REJECT_DATA			0x0009	//拒绝数据
#define	CMD_CHANGE_MEDIA		0x000A	//媒体改变	---Ex1中不支持


#define CMD_CONNECT_R			0x8001	//连接指令
#define CMD_FORWARD_R			0x8002	//前转指令
#define CMD_PUSH_HEADER_R		0x8003	//推送头
#define CMD_PUSH_SERVANTS_R		0x8004	//推送Servants
#define	CMD_LEAVE_R				0x8005	//离开
#define	CMD_ACTIVE_R			0x8006	//激活
#define CMD_APPLY_DATAS_R		0x8007	//请求数据
#define	CMD_PUSH_DATA_R			0x8008	//推送数据
#define CMD_REJECT_DATA_R		0x8009	//拒绝数据
#define	CMD_CHANGE_MEDIA_R		0x800A	//媒体改变

typedef struct _CMD_TAG
{
	WORD wCmd;	//Cmd
	WORD wSeq;	//Sequence
	char* buf;	
	WORD buflen;
	sockaddr_in addr;// only for servant manager
} CMD;
enum ConnectResult
{
	Connect_Result_OK ,
	Content_Result_ReDirection ,
	Content_Result_Require_New_Version
};
//协议类
namespace Protocol
{
	
	inline WORD getSeq()
	{
		static WORD _seq =0;
		
		return _seq ++;
		
	};

	CMD getConnect(DWORD sid,const char *ChID);
	//获取连接的反馈。当Result ==Connect_Result_ReDirection时，ip和port参数有效；Result == Connect_Result_Require_New_Version时,ver参数有效
	CMD getConnectR(WORD wSeq,ConnectResult Result,DWORD ip = 0,WORD port =0);

	void parseConnect(const char* buf,int buflen,char** ip,WORD *port,char **ChID);

};*/