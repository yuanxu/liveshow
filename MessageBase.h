#pragma once
#include "sys.h"
#include <time.h>

#define CMD_UNKNOWN				0x0000	//未知
#define CMD_JOIN				0x0001	//加入指令
//#define CMD_FORWARD				0x0002	//前转指令			'Forward指令无意义?
#define CMD_PUSH_HEADER			0x0003	//推送头
#define CMD_PUSH_SERVANTS		0x0004	//推送Servants  
#define	CMD_LEAVE				0x0005	//离开
#define	CMD_EXCHANGE_BM			0x0006	//BufferMap交换信息
#define CMD_APPLY_DATAS			0x0007	//请求数据
#define	CMD_PUSH_DATA			0x0008	//推送数据
#define CMD_REJECT_DATA			0x0009	//拒绝数据	---Ex1中不支持
#define	CMD_CHANGE_MEDIA		0x000A	//媒体改变	---Ex1中不支持
#define CMD_PING				0x000B	//Ping	用于更新mCache			'要不要走UDP协议?
#define CMD_CONNECT				0x000C	//连接请求

#define CMD_APPLY_PACKET		0x000E	//请求特定的数据包


#define CMD_JOIN_R				0x8001	//加入指令
//#define CMD_FORWARD_R			0x8002	//前转指令
#define CMD_PUSH_HEADER_R		0x8003	//推送头
#define CMD_PUSH_SERVANTS_R		0x8004	//推送Servants
#define	CMD_LEAVE_R				0x8005	//离开
#define	CMD_EXCHANGE_BM_R		0x8006	//激活
#define CMD_APPLY_DATAS_R		0x8007	//请求数据
#define	CMD_PUSH_DATA_R			0x8008	//推送数据
#define CMD_REJECT_DATA_R		0x8009	//拒绝数据
#define	CMD_CHANGE_MEDIA_R		0x800A	//媒体改变
#define CMD_CONNECT_R			0x800C	//连接响应
//



//////////////////////////////////////////////////////////////////////////
//Proxy Protocol
#define PXY_UNKNOWN				0x0000	//未知
#define PXY_HELLO				0x0001	//Shakehands
//#define PXY_PING				0x0002	//激活
#define PXY_CALLBACK			0x0003	//call back request
#define PXY_LEAVE				0x0004	
#define CMD_SYNC_ID				0x000D	//同步SegmentID					'通过UDP，减少服务器压力

#define PXY_HELLO_R				0x8001	//Shakehands
//#define PXY_PING_R				0x8002	//激活
#define PXY_CALLBACK_R			0x8003	//call back request
#define CMD_SYNC_ID_R			0x800D	//同步SegmentID的响应


#define VERSION 0x0100



////////////////////////////////////////////////////////////////////////////////////////////////////

enum NetworkType
{
	NetType_Public,NetType_Private
};

class SocketObj;
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ServantInfo_TAG
{
	//此节点监听地址
	sockaddr_in addr;
	DWORD dwSID;
	//WORD wSeqID;
	//time_t tLastUpdate,tLastPing; move to SocketObj
	SocketObj *lpSktObj ;
	bool bIsMySelf;
	volatile DWORD dwRecvCount;			//接收到的Segment数量
	volatile DWORD dwSentCount;			//发送成功的Segment数量

	ServantInfo_TAG()
	{
		dwSID = 0;
		//wSeqID = 0;
		lpSktObj = NULL;
		bIsMySelf = false;
		dwRecvCount = dwSentCount =0;
	}
public:
	//重载=运算符
	bool operator =(const  ServantInfo_TAG &p) const
	{
		return dwSID == p.dwSID;
	};

	//重载<运算符
	bool operator <(const  ServantInfo_TAG &p) const
	{
		return max(dwRecvCount,dwSentCount) < max(p.dwRecvCount,p.dwSentCount) ;
	};

} ServantInfo;


////////////////////////////////////////////////////////////////////////////////////////////////////

class MessageBase
{
public:

	MessageBase(void)
		:wCMD(CMD_UNKNOWN)
		,wSeqID(0)

	{
		lpBuf = NULL;
		bNeedConfirm=false;

		bIsControl = false;
		dwChID = 0;
	};

	
	MessageBase(WORD cmd)
		:wCMD(cmd)
		,wSeqID(0)

	{
		lpBuf = NULL;
		bNeedConfirm=false;
		
		bIsControl = false;
		dwChID = 0;
	};

	MessageBase(const char* p,const int len)		
	{
		fromBytes(p,len);
		lpBuf = NULL;
		bNeedConfirm=false;
		
		bIsControl = false;
		dwChID = 0;
	};
	virtual ~MessageBase(void)
	{
		if(NULL != lpBuf)
			delete[] lpBuf;
		
	};

private:
	static WLock *m_slock;
	static WORD	m_sSeqID;
public:
	WORD wCMD;		//命令字
	WORD wSeqID;	//序列号
	DWORD dwChID;	//频道ID	
	
	char *lpBuf;		//Buf
	int	 bufLen;		//BufLen
	

	bool bNeedConfirm ; //是否需要返回确认消息
	bool bIsControl;


	//从字节数组恢复对象
	virtual void fromBytes(const char* p,const int len) = 0;

	//将对象转换为字节数组
	virtual void toBytes(void) =0;

};

/************************************************************************/
/* Reliable Message Base.                                               */
/************************************************************************/
//struct rtt_info;

class ReliableMessageBase :
	public MessageBase
{
public:
	DWORD dwRetryTime;
	time_t	tSendTime;

public:
	ReliableMessageBase(void):MessageBase()
	{
		bNeedConfirm =  true;
		dwRetryTime = 0;
			
	};

	ReliableMessageBase(WORD cmd):MessageBase(cmd)
	{
		bNeedConfirm = true;
		dwRetryTime = 0;
	};

	ReliableMessageBase(const char* p,const int len):MessageBase(p,len)
	{
		bNeedConfirm = true;
		dwRetryTime = 0;
	};

	virtual ~ReliableMessageBase(void)
	{
	};

	//从字节数组恢复对象
	virtual void fromBytes(const char* p,const int len) = 0;

	//将对象转换为字节数组
	virtual void toBytes(void) =0;

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ACKMessageBase :
	public MessageBase
{

public:
	DWORD dwRetryTime;
	time_t	tSendTime;
public:
	ACKMessageBase(void):MessageBase()
	{
		bNeedConfirm =  false;
		dwRetryTime = 0;
	};

	ACKMessageBase(WORD cmd):MessageBase(cmd)
	{
		bNeedConfirm = false;
		dwRetryTime = 0;
	};

	ACKMessageBase(const char* p,const int len):MessageBase(p,len)
	{
		bNeedConfirm = false;
		dwRetryTime = 0;
	};

	virtual ~ACKMessageBase(void)
	{
	};

		//从字节数组恢复对象
	virtual void fromBytes(const char* p,const int len) = 0;

	//将对象转换为字节数组
	virtual void toBytes(void) =0;

};
