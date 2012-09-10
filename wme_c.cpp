#pragma once

#include "StdAfx.h"
#include "wme_c.h"
#include "sys.h"
#include "stream.h"
#include "common.h"
#include "asf.h"

const char* HTTP_REQ_CMD = "GET / HTTP/1.0\r\n";
const char* HTTP_REQ_AGENT = "User-Agent: NSPlayer/4.1.0.3925\r\n";
const char* HTTP_REQ_PRAGMA = "no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=1,max-duration=0\r\nxClientGUID={2200AD50-2C39-46c0-AE0A-2CA76D8C766D}\r\n";
const char* HTTP_REQ_PRAGMA2 ="Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=4294967295:4294967295,request-context=2,max-duration=2147609515\r\nPragma: xPlayStrm=1\r\nPragma: xClientGUID={2200AD50-2C39-46c0-AE0A-2CA76D8C766D}\r\nPragma: stream-switch-count=2\r\nPragma: stream-switch-entry=ffff:1:0 ffff:2:0\r\n";
wme_c::~wme_c(void)
{
}

void wme_c::Start(char *ip, WORD port)
{
	//放到一个线程里
	
	lpInfo.lpIP = ip;
	lpInfo.wPort = port;
	lpInfo.lpInstance = this;

	lpThrd = new ThreadInfo();
	lpThrd->data = &lpInfo;
	
	lpThrd->func = (THREAD_FUNC)DataProcessFunc;
	sys->startThread(lpThrd);
	
}

int wme_c::DataProcessFunc(ThreadInfo *lpInfo)
{
	IpInfo* lpIP = ((IpInfo*)lpInfo->data);
	char *ip = lpIP->lpIP;
	WORD port = lpIP->wPort;
	
	while(1)
	{

		lpIP->lpInstance->getHeader(ip,port);

		lpIP->lpInstance->processData(ip,port);
		//lpIP->lpInstance->receive(ip,port);
	}
	return 0;
}
char* wme_c::getHeader(char* ip,WORD port)
{
	Buffer* lpBuf = m_buf;
	SOCKET skt = sys->createSocket(SOCK_STREAM);
	sockaddr_in clientService; 
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( ip);
	clientService.sin_port = htons( port);
	DWORD e ;
	if(connect(skt,(SOCKADDR*) &clientService,sizeof(clientService))==SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK)
	{		
		e=GetLastError();
		throw StreamException("wme_c::getHeader.connect ",e);
		//return NULL;
	}
	MemoryStream *stream  =new MemoryStream(1024);
	memset(stream->buf,0,1024);
	stream->write(HTTP_REQ_CMD, static_cast<int>( strlen(HTTP_REQ_CMD)  ));
	stream->write(HTTP_REQ_AGENT,static_cast<int>(strlen(HTTP_REQ_AGENT) ) );
	stream->write(HTTP_REQ_PRAGMA,static_cast<int>(strlen(HTTP_REQ_PRAGMA))  );
	stream->writeChar('\r');
	stream->writeChar('\n');

	if(send(skt,stream->buf,static_cast<int>(strlen(stream->buf)),0)==SOCKET_ERROR)
	{
		e = GetLastError();
		throw StreamException("wme_c::getHeader.send ",e);
		//return NULL;
	}
	delete stream;
	 //处理接收
	char buf[8192];
	char *lpRet = NULL;//= new char;
	int iGet= 0,iPos = 0;
	WORD len = 0;
	while(true) //读取数据到尾
	{
		int l =0;
		if( (l = recv(skt,buf+iGet,sizeof(buf) - iGet,0)) == SOCKET_ERROR)
		{
			e = GetLastError();
			throw StreamException("wme_c::getHeader.recv ",e);
		}
		if( 0 == l)
		{
			//throw StreamException("wme_c::getHeader.socetclosed ");
			break;
		}
		iGet +=l ;
	}
		
	if(strstr(buf,"OK") !=NULL) // get ok
	{
		char* pLoc = strstr(buf,"\r\n\r\n");
		pLoc +=4;//移动到\r\n\r\n后边，进入实际的包体

		if( 0x24 == pLoc[0]  && 0x48 == pLoc[1]) //header
		{
			
			pLoc += 2;
			memcpy(&len,pLoc,2);
			len -= 8;
			//&len  pLoc[2] < 8 & pLoc[3]; //get length
			lpRet = new char[len];
			pLoc += 10 ;
			memcpy(lpRet,pLoc,len);

			

			//设置Segment::setSegmentSize;
			//memcpy(&dwPacketSize,pLoc + HDR_ASF_PACKET_SIZE,4);
			//memcpy(&dwBitRate,pLoc + HDR_ASF_BITRATE,4);
			asfHeader *objHeader = new asfHeader(pLoc,len);
			dwPacketSize = objHeader->getChunkSize();
			dwBitRate = objHeader->getBitRate();
			delete objHeader;
			int size = ( dwBitRate /8 /dwPacketSize)*dwPacketSize;//(349056 /8 /0x5a4) * 0x5a4;
			Segment::setSegmentSize( size);//试验用 1.06M
			
			lpBuf->m_Header.lpData = lpRet;
			lpBuf->m_Header.len = len;
			lpBuf->Init(0);
		}
		
		
	}
	else
	{
		//error
		return NULL;
	}
		
	
	closesocket(skt);

	return lpRet;
}


void wme_c::receive(char* ip,WORD port)
{
	Buffer* lpBuf = m_buf;
	SOCKET skt = sys->createSocket(SOCK_STREAM);
	sockaddr_in clientService; 
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( ip);
	clientService.sin_port = htons( port);
	DWORD e;

	int iReqNum = 1;	//请求次数
	bool bCnnected = false;

	const int BUF_SIZE = 64*1024 +12 ;// 8192; //64KB
	char buf[BUF_SIZE];
	int iGet = 0;
	int iPos = 0;
	bool bInited = false;
	while (true)
	{
		
		if (!bCnnected) //not connected
		{
			if(connect(skt,(SOCKADDR*) &clientService,sizeof(clientService))==SOCKET_ERROR)
			{		
				e=GetLastError();
				throw StreamException("wme_c::getHeader.connect ",e);
				//return NULL;
			}	
			MemoryStream *stream  =new MemoryStream(2048);
			if (iReqNum % 2 != 0) //第一次请求
			{
				memset(stream->buf,0,stream->len);
				stream->write(HTTP_REQ_CMD, static_cast<int>( strlen(HTTP_REQ_CMD)  ));
				stream->write(HTTP_REQ_AGENT,static_cast<int>(strlen(HTTP_REQ_AGENT) ) );
				stream->write(HTTP_REQ_PRAGMA,static_cast<int>(strlen(HTTP_REQ_PRAGMA))  );
				stream->writeChar('\r');
				stream->writeChar('\n');
			} 
			else //第二次请求
			{
				
				memset(stream->buf,0,stream->len);
				stream->write(HTTP_REQ_CMD, static_cast<int>(strlen(HTTP_REQ_CMD)  ));
				stream->write(HTTP_REQ_AGENT,static_cast<int>(strlen(HTTP_REQ_AGENT) ) );
				stream->write(HTTP_REQ_PRAGMA2,static_cast<int>(strlen(HTTP_REQ_PRAGMA2))  ); //第二次请求
				stream->writeChar('\r');
				stream->writeChar('\n');
			}
			if(send(skt,stream->buf,static_cast<int>(strlen(stream->buf)),0)==SOCKET_ERROR)
			{
				e = GetLastError();

				return ;
			}
			delete stream;
		} 
		
		Sleep(10);
lblRecv:
		if (iPos > 0)
		{
			memcpy(buf,buf+iPos,iGet - iPos);
			iGet -= iPos;
			iPos = 0;
		}
		int iRecv = recv(skt,buf+iGet,BUF_SIZE - iGet,0);
		if (0 == iRecv)
		{
			//net workerror
		}
		iGet += iRecv;

		char* HTTP = "HTTP";
		
		char* RNRN = "\r\n\r\n";
		if (memfind(buf,iGet,HTTP,4) != NULL)
		{
			iPos = ( memfind(buf,iGet,RNRN,4)-buf) + 4;
		}
		AsfChunkHeader chkHeader;
		memcpy(&chkHeader,buf+iPos,sizeof(chkHeader));
		if (chkHeader.wConfirmLen != chkHeader.wLen)
		{
			//TODO:error
		}
		if (chkHeader.wLen + 4> iGet - iPos )
		{
			goto lblRecv;
		}

		switch(chkHeader.wCMD)
		{
		case 0x4824://"$H"
			{
				if (!bInited)
				{
					asfHeader *objHeader = new asfHeader(buf+iPos + 4 + 8,chkHeader.wLen);
					dwPacketSize = objHeader->getChunkSize();
					dwBitRate = objHeader->getBitRate();
					delete objHeader;
					int size = ( dwBitRate /8 /dwPacketSize)*dwPacketSize;//(349056 /8 /0x5a4) * 0x5a4;
					Segment::setSegmentSize( size);//试验用 1.06M

					lpBuf->m_Header.lpData = new char[chkHeader.wLen - 8];
					memcpy(lpBuf->m_Header.lpData,buf + iPos + 4 + 8 ,chkHeader.wLen -  8);
					lpBuf->m_Header.len = chkHeader.wLen - 8;
					lpBuf->Init(0);
					bInited = true;
				}
				bCnnected =true;
				iPos +=  chkHeader.wLen + 4;
			}
			break;
		case 0xa444://?D
		case 0x4424://$D
			{
				iPos = processData(lpBuf,buf,iPos,iGet);
				if(iPos<0)
				{
					bCnnected = false;
					closesocket(skt);
				}
			}
		    break;
		
		}
		
	}

}

//must process one asf chunk only!!!
int wme_c::processData(Buffer *lpBuf, char* buf,int iPos,int iGet)
{
	static int cSeqID = 0 ;//从第三秒开始 。用以缓冲。 //TODO:可以从BM中得到当前ID然后加一个缓冲时间
	static byte cSeq = 0;	//live show packet 中的序号
	 int iAsf = 0; //当前asf chunk大小
	 char buf_t [PACKET_DATA_SIZE];
	 int iOffset_t = 0;
	 int len;
	 int iNeedData = 0 ; //当前数据包需要的数据
	if (cSeq != 0) //重新填充此segment
	{
		cSeq = 0;
	}
	static int iPadLen = 0;// Padding data length
	
		while(iPos < iGet ) //数据处理循环.iPos当前数据处理指针，iGet，当前可用数据长度
		{
			int iAvailable = iGet - iPos; //接收缓冲中可用数据

			if(iAvailable <= 12)
				break; //跳出，从wme中读取后续数据。
			if((char)0x24 == buf[iPos]  && (char)0x44 == buf[iPos + 1]) //a new asf chunk
			{
				if (memcmp(buf+iPos+2,buf+iPos+10,2)!=0)//len != confirm len
				{
					std::cout << endl << "ERROR!!Read Http Streaming chunk Error!len != confirm len.ERROR!!" << endl;
					
					return -1;
				}

				memcpy(&len,buf + iPos +2 ,2);//得到数据的报大小
				len -= 8;//http stream中的长度= 数据长度加上数据头长度(8字节)
				assert(len > 0);

				iAsf = len ; 

				//去除 http streaming 头 和数据头长度4+8
				iPos += 12;
				iAvailable -= 12;
				//fix asf parse info 's padding length
				
				//以下计算内容不正确，没有处理Error correction data(82 00 00)

				//假设每个包以0x 82 00 00 开始，则接下来一个包就是asf parse info的第一个字节
				int iSkip =0;

				//assert((char)0x82==buf[iPos] && (char)0 == buf[iPos+1] && (char)0 == buf[iPos+2]);

				char c= (buf[iPos +3] & 0x60) ; //判断 Packet Length Type是否存在
				c = c>> 5 ;	

				switch(c)
				{
				case 0:
					iSkip =0;
					break;
				case 1:
					iSkip = 1;
					//TODO:最大256字节一个包？好像不会出现这种情况
#if _DEBUG
					log::WriteLine("Packet Length is 0x01(Byte)");
#endif
					memcpy(buf+iPos +5,&dwPacketSize,1);
					break;
				case 2: //340Kbps左右都0x5a4的大小
					iSkip = 2;
					memcpy(buf+iPos +5,&dwPacketSize,2);
					break;
				case 3:
					iSkip = 4;
					memcpy(buf+iPos +5,&dwPacketSize,4);
					break;
				}
				c= (buf[iPos +3] & 0x06) ; //判断 Sequence Length Type是否存在
				c = c>> 5 ;	
				switch(c)
				{
				case 0:
					iSkip += 0 ;
					break;
				case 1:
					iSkip += 1;
					break;
				case 2:
					iSkip += 2;
					break;
				case 3:
					iSkip += 4;
					break;
				}
				//这段程序假设Padding Type = 01 ，即Byte。如果出现非Byte，会有错误！ 
				if(iAsf != dwPacketSize)	//计算padding　
				{
					buf[iPos + 5 + iSkip ] = (byte)(dwPacketSize - iAsf); //修改asf流中padding尺寸
					//在这里应该计算出iPadlen的大小???
					//iPadLen = dwPacketSize - iAsf;
					
				iPadLen = 0;
				} //end of iAsf != dwPacketSize
				else
				{
					assert(buf[iPos + 5 + iSkip] == 0);
				}
				

			}

			if( 0 == iNeedData) //当前数据包需要的数据为0，需要重新计算数据包所需数量
			{
				if(cSeq < Segment::getPacketsOfSegment() - 1)
				{
					//检查读到的数据是否足够push，如果不够，则启动一个读
					iNeedData = PACKET_DATA_SIZE;
				}
				else
				{
					iNeedData = Segment::getLastPacketSize();
				}
				//此时iOffset == 0
				assert(0 == iOffset_t);
				//如果有上一个包未容纳的padding数据，则将padding数据补到当前的cSeqment中
				while(iPadLen >0)
				{
					buf_t[iOffset_t] = 0;
					iOffset_t ++;
					iPadLen --;
				}
			}
			assert(iNeedData >= iOffset_t);
			if(iAvailable > (iNeedData - iOffset_t)) //主缓冲中的数据多余需求数据
			{
				if(iNeedData - iOffset_t >= iAsf) //需求的数据比一个asf chunk大或正好需要chunk
				{
					//将此asf复制到buf_t
					memcpy(buf_t + iOffset_t,buf + iPos,iAsf);
					iOffset_t += iAsf;
					iPos += iAsf;
					iAsf = len; //len 当前asf chunk长度。this is  necessary for next if sentence
					iPadLen = dwPacketSize - len; //padding data length

				}
				else //用不了一个asf chunk就可将当前but_t缓存区添满
				{
					memcpy(buf_t + iOffset_t,buf + iPos,iNeedData - iOffset_t);//复制一部分数据
					iPos += iNeedData - iOffset_t;
					iAsf -= iNeedData - iOffset_t;//iAsf chunk中剩余字节

					iOffset_t += iNeedData - iOffset_t;
				}

				if(len == iAsf && 0 != iPadLen && iOffset_t != iNeedData  ) //需要填充，且当前Segment有空间
				{
					while( iOffset_t != iNeedData && iPadLen >0 )
					{
						buf_t[iOffset_t] = 0;
						iOffset_t++;
						iPadLen --;
					}
					//执行完以上循环后，还可能剩余一部分padding数据，这个时候iPadLen>0，iOffset_t == iNeedData
					
				}

				//看数据是否已填满
				if(iOffset_t == iNeedData)
				{

#if _DEBUG
					if (0 == cSeq)
					{
						assert((char)0x82==buf_t[0] && (char)0 == buf_t[1] && (char)0 == buf_t[2]);
					}
#endif

					lpBuf->srv_PushData(cSeqID,cSeq,buf_t,iNeedData);

					if(cSeq == Segment::getPacketsOfSegment() -1) //最后一个包
					{
						//调整ID
						cSeq = 0;
						if(cSeqID == 0xFFFF)
							cSeqID = 0;
						else
							cSeqID++;
					}
					else
					{
						cSeq++;
					}
					iOffset_t = 0;
					iNeedData = 0; 
				}
				if(iAsf == len) //已经处理完一个chunk
				{
					return iPos;
				}		

			} 
			else //数据不足
			{
				if (iAsf <= iAvailable)
				{
					memcpy(buf_t+iOffset_t,buf+iPos,iAsf);
					iOffset_t+=iAsf;
					iPos += iAsf;
					iAsf -= iAsf;

				}
				else
				{
					memcpy(buf_t+iOffset_t,buf+iPos,iAvailable);;
					iOffset_t+=iAvailable;
					iPos += iAvailable;
					iAsf -= iAvailable;

				}
				if (iAsf ==0 )
				{
					iPadLen = dwPacketSize - len;
				}
				break;
			}//end of 主缓冲

		} //end of 数据处理循环

		//调整缓冲区

		assert(iGet >= iPos);

	
	return iPos;
}
void wme_c::processData(char* ip,WORD port)
 {
	 
	 Buffer* lpBuf = m_buf;
	 SOCKET skt = sys->createSocket(SOCK_STREAM);
	sockaddr_in clientService; 
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( ip);
	clientService.sin_port = htons( port);
	DWORD e;
	if(connect(skt,(SOCKADDR*) &clientService,sizeof(clientService))==SOCKET_ERROR)
	{		
		e=GetLastError();
		return ;
	}
	MemoryStream *stream  =new MemoryStream(2048);
	memset(stream->buf,0,1024);
	stream->write(HTTP_REQ_CMD, static_cast<int>(strlen(HTTP_REQ_CMD)  ));
	stream->write(HTTP_REQ_AGENT,static_cast<int>(strlen(HTTP_REQ_AGENT) ) );
	stream->write(HTTP_REQ_PRAGMA2,static_cast<int>(strlen(HTTP_REQ_PRAGMA2))  ); //第二次请求
	stream->writeChar('\r');
	stream->writeChar('\n');

	if(send(skt,stream->buf,static_cast<int>(strlen(stream->buf)),0)==SOCKET_ERROR)
	{
		e = GetLastError();
		
		return ;
	}
	delete stream;
	
	//处理数据
 	const int BUF_SIZE = 64*1024 +12 ;// 8192; //64KB
	char buf[BUF_SIZE];
	int iGet = 0;
	Sleep(10);
	iGet = recv(skt,buf,17,0); //读取状态行
	if(0 == iGet)
	{
		e = GetLastError();
		return;
	}
	buf[18]='\0';

	if(strstr(buf,"OK")==NULL) // not got HTTP/1.x 200 OK
	{
		return;
	}
	iGet = recv(skt,buf,BUF_SIZE,0); //跳过头
	if(iGet == 0)
	{
		iGet = recv(skt,buf,BUF_SIZE,0); //跳过头
	}
	char* lpLoc = strstr(buf,"\r\n\r\n");
	if(lpLoc != NULL)
		lpLoc += 4;
	memcpy(buf,lpLoc,iGet - (lpLoc - buf )); //移动数据，覆盖http头部
	iGet -= (lpLoc - buf  ); 
	WORD len;
	memcpy(&len,buf+2,2); //get asf header len

	//跳过第二次传来的头数据
	while(len + 4 > iGet) //数据不够时，读取数据
	{
		int i = recv(skt,buf + iGet,BUF_SIZE - iGet,0);
		if(i == SOCKET_ERROR)
		{
			e = GetLastError();
			return;
		}
		if(0 == i)
		{
			e = GetLastError();
			return;//连接已断开
		}
		iGet +=i;
	}
	
	memcpy(buf,buf + len,iGet - (len +4)); //跳过asf 头 +4 = http streaming 's header
	iGet -= (len +4);

	len = 0;
	int iPos = 0;
	
	static int cSeqID = 0 ;//从第三秒开始 。用以缓冲。 //TODO:可以从BM中得到当前ID然后加一个缓冲时间
	byte cSeq = 0;	//live show packet 中的序号
	int iAsf = len; //当前asf chunk大小
	char buf_t [PACKET_DATA_SIZE];
	int iOffset_t = 0;
	int iNeedData = 0 ; //当前数据包需要的数据
		
	cSeq=0;
	assert(cSeq == 0);
	int iPadLen = 0;// Padding data length
	while(true)  //接收循环
	{
		while(iPos < iGet) //数据处理循环.iPos当前数据处理指针，iGet，当前可用数据长度
		{
			int iAvailable = iGet - iPos; //接收缓冲中可用数据

			if(iAvailable <= 12)
				break; //跳出，从wme中读取后续数据。
			if((char)0x24 == buf[iPos]  && (char)0x44 == buf[iPos + 1]) //a new asf chunk
			{
				if (memcmp(buf+iPos+2,buf+iPos+10,2)!=0)//len != confirm len
				{
					std::cout << endl << "ERROR!!Read Http Streaming chunk Error!len != confirm len.ERROR!!" << endl;
					closesocket(skt);
					return;
				}
				memcpy(&len,buf + iPos +2 ,2);//得到数据的报大小
				len -= 8;//http stream中的长度= 数据长度加上数据头长度(8字节)
				assert(len > 0);
				
				iAsf = len ; 

				//去除 http streaming 头 和数据头长度4+8
				iPos += 12;
				iAvailable -= 12;
				//fix asf parse info 's padding length

				
					//假设每个包以0x 82 00 00 开始，则接下来一个包就是asf parse info的第一个字节
					int iSkip =0;

					//assert((char)0x82==buf[iPos] && (char)0 == buf[iPos+1] && (char)0 == buf[iPos+2]);

					char c= (buf[iPos +3] & 0x60) ; //判断 Packet Length Type是否存在
					c = c>> 5 ;	
					
					switch(c)
					{
						case 0:
							iSkip =0;
							break;
						case 1:
							iSkip = 1;
							//TODO:最大256字节一个包？好像不会出现这种情况
#if _DEBUG
							log::WriteLine("Packet Length is 0x01(Byte)");
#endif
							memcpy(buf+iPos +5,&dwPacketSize,1);
							break;
						case 2: //340Kbps左右都0x5a4的大小
							iSkip = 2;
							memcpy(buf+iPos +5,&dwPacketSize,2);
							break;
						case 3:
							iSkip = 4;
							memcpy(buf+iPos +5,&dwPacketSize,4);
							break;
					}
					c= (buf[iPos +3] & 0x06) ; //判断 Sequence Length Type是否存在
					c = c>> 5 ;	
					switch(c)
					{
						case 0:
							iSkip += 0 ;
							break;
						case 1:
							iSkip += 1;
							break;
						case 2:
							iSkip += 2;
							break;
						case 3:
							iSkip += 4;
							break;
					}
				//这段程序假设Padding Type = 01 ，即Byte。如果出现非Byte，会有错误！ 
				if(iAsf != dwPacketSize)	//计算padding　
				{
					buf[iPos + 5 + iSkip ] = (byte)(dwPacketSize - iAsf); //修改asf流中padding尺寸
					//在这里应该计算出iPadlen的大小???
					//iPadLen = dwPacketSize - iAsf;

				} //end of iAsf != dwPacketSize
				else
				{
					//assert(buf[iPos + 5 + iSkip] == 0);
				}


			}

			if( 0 == iNeedData) //当前数据包需要的数据为0，需要重新计算数据包所需数量
			{
				if(cSeq < Segment::getPacketsOfSegment() - 1)
				{
					//检查读到的数据是否足够push，如果不够，则启动一个读
					iNeedData = PACKET_DATA_SIZE;
				}
				else
				{
					iNeedData = Segment::getLastPacketSize();
				}
				//此时iOffset == 0
				assert(0 == iOffset_t);
				//如果有上一个包未容纳的padding数据，则将padding数据补到当前的cSeqment中
				while(iPadLen >0)
				{
					buf_t[iOffset_t] = 0;
					iOffset_t ++;
					iPadLen --;
				}
			}
			assert(iNeedData >= iOffset_t);
			if(iAvailable > (iNeedData - iOffset_t)) //主缓冲中的数据多余需求数据
			{
				if(iNeedData - iOffset_t >= iAsf) //需求的数据比一个asf chunk大或正好需要chunk
				{
					//将此asf复制到buf_t
					memcpy(buf_t + iOffset_t,buf + iPos,iAsf);
					iOffset_t += iAsf;
					iPos += iAsf;
					iAsf = len; //len 当前asf chunk长度。this is  necessary for next if sentence
					iPadLen = dwPacketSize - len; //padding data length

				}
				else //用不了一个asf chunk就可将当前but_t缓存区添满
				{
					memcpy(buf_t + iOffset_t,buf + iPos,iNeedData - iOffset_t);//复制一部分数据
					iPos += iNeedData - iOffset_t;
					iAsf -= iNeedData - iOffset_t;//iAsf chunk中剩余字节
					
					iOffset_t += iNeedData - iOffset_t;
				}
				
				if(len == iAsf && 0 != iPadLen && iOffset_t != iNeedData  ) //需要填充，且当前Segment有空间
				{
					while( iOffset_t != iNeedData && iPadLen >0 )
					{
						buf_t[iOffset_t] = 0;
						iOffset_t++;
						iPadLen --;
					}
					//执行完以上循环后，还可能剩余一部分padding数据，这个时候iPadLen>0，iOffset_t == iNeedData
				}

				//看数据是否已填满
				if(iOffset_t == iNeedData)
				{
					
#if _DEBUG
					if (0 == cSeq)
					{
						assert((char)0x82==buf_t[0] && (char)0 == buf_t[1] && (char)0 == buf_t[2]);
					}
#endif
					
					lpBuf->srv_PushData(cSeqID,cSeq,buf_t,iNeedData);

					if(cSeq == Segment::getPacketsOfSegment() -1) //最后一个包
					{
						//调整ID
						cSeq = 0;
						if(cSeqID == 0xFFFF)
							cSeqID = 0;
						else
							cSeqID++;
					}
					else
					{
						cSeq++;
					}
					iOffset_t = 0;
					iNeedData = 0; 
				}
				
			} 
			else //数据不足
			{
				break;
			}//end of 主缓冲
			
		} //end of 数据处理循环
		
		//调整缓冲区

		assert(iGet >= iPos);

		if(iGet - iPos >0)
			memcpy(buf,buf + iPos,iGet - iPos);
		iPos = iGet - iPos;
		
		assert(iPos <= BUF_SIZE);

		iGet = recv(skt,buf + iPos,BUF_SIZE - iPos,0); //启动接收		
		
		assert(iGet <= BUF_SIZE - iPos);

		if (0 == iGet)
			break; //无可用数据，连接已关闭
		iGet += iPos; //iGet表示读到的总数
		iPos = 0;
	}//end of 接收循环
 }