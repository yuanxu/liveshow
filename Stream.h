#pragma once
#include "common.h"
#include "sys.h"
// -------------------------------------
;class Stream
{
public:
	Stream()
	:writeCRLF(true)
	,totalBytesIn(0)
	,totalBytesOut(0)
	,lastBytesIn(0)
	,lastBytesOut(0)
	,bytesInPerSec(0)
	,bytesOutPerSec(0)
	,lastUpdate(0)
	,bitsBuffer(0)
	,bitsPos(0)
	{
	}

	virtual int readUpto(void *,int) {return 0;}
	virtual int read(void *,int)=0;
	virtual void write(const void *,int) = 0;
    virtual bool eof()
    {
    	throw StreamException("Stream can`t eof");
		return false;
    }

	virtual void rewind()
	{
    	throw StreamException("Stream can`t rewind");
	}

	virtual void seekTo(int)
	{
    	throw StreamException("Stream can`t seek");
	}

	void writeTo(Stream &out, int len);
	virtual void skip(int i);

	virtual void close()
	{
	}

	virtual void	setReadTimeout(unsigned int ) 
	{
	}
	virtual void	setWriteTimeout(unsigned int )
	{
	}
	virtual void	setPollRead(bool)
	{
	}

	virtual int		getPosition() {return 0;}


	// binary
    char	readChar()
    {
    	char v;
        read(&v,1);
        return v;
    }
    short	readShort()
    {
    	short v;
        read(&v,2);
		CHECK_ENDIAN2(v);
        return v;
    }
	WORD readWORD()
	{
		WORD v;
        read(&v,2);
		CHECK_ENDIAN2(v);
        return v;
	}
	DWORD readDWORD()
	{
		DWORD v;
        read(&v,4);
		CHECK_ENDIAN4(v);
        return v;
	}
    long	readLong()
    {
    	long v;
        read(&v,4);
		CHECK_ENDIAN4(v);
        return v;
    }
	int readInt()
	{
		return readLong();
	}
	
	int	readInt24()
	{
		int v=0;
        read(&v,3);
		CHECK_ENDIAN3(v);
	}



	long readTag()
	{
		long v = readLong();
		return SWAP4(v);
	}

	int	readString(char *s, int max)
	{
		int cnt=0;
		while (max)
		{
			char c = readChar();
			*s++ = c;
			cnt++;
			max--;
			if (!c)
				break;
		}
		return cnt;
	}

	void readString(char *s)
	{
		int cnt=0;
		while (true)
		{
			char c = readChar();
			*s++ = c;
			cnt++;
			if (!c)
				break;
		}
	//	s='\0';
	}

	virtual bool	readReady() {return true;}
	virtual int numPending() {return 0;}


	

	void	writeChar(char v)
	{
		write(&v,1);
	}
	void writeWORD(WORD v)
	{
		CHECK_ENDIAN2(v);
		write(&v,2);
	}
	void writeDWORD(DWORD v)
	{
		CHECK_ENDIAN4(v);
		write(&v,4);
	}

	void	writeShort(short v)
	{
		CHECK_ENDIAN2(v);
		write(&v,2);
	}

	void	writeLong(long v)
	{
		CHECK_ENDIAN4(v);
		write(&v,4);
	}
	void writeInt(int v) {writeLong(v);}

	void	writeTag(long v)
	{
		//CHECK_ENDIAN4(v);
		writeLong(SWAP4(v));
	}

	void	writeTag(char id[4])
	{
		write(id,4);
	}

	int	writeUTF8(unsigned int);

	// text
    int	readLine(char *in, int max);

    int		readWord(char *, int);
	int		readBase64(char *, int);

	void	write(const char *,va_list);
	void	writeLine(const char *);
	void	writeLineF(const char *,...);
	void	writeString(const char *);
	void	writeStringF(const char *,...);

	bool	writeCRLF;

	int		readBits(int);

	void	updateTotals(unsigned int,unsigned int);


	unsigned char bitsBuffer;
	unsigned int bitsPos;

	unsigned int totalBytesIn,totalBytesOut;
	unsigned int lastBytesIn,lastBytesOut;
	unsigned int bytesInPerSec,bytesOutPerSec;
	unsigned int lastUpdate;

};


// -------------------------------------
class MemoryStream : public Stream
{
public:
	MemoryStream()
	:buf(NULL)
	,len(0)
	,pos(0)
	{
	}

	MemoryStream(void *p, int l)
	:buf((char *)p)
	,len(l)
	,pos(0)
	{
	}

	MemoryStream(int l)
	:buf(new char[l])
	,len(l)
	,pos(0)
	{
	}

	
	void free()
	{
		if (buf)
		{
			delete buf;
			buf = NULL;
		}

	}

	virtual int read(void *p,int l)
    {
		if (pos+l <= len)
		{
			memcpy(p,&buf[pos],l);
			pos += l;
			return l;
		}else
		{
			memset(p,0,l);
			return 0;
		}
    }

	virtual void write(const void *p,int l)
    {
		if ((pos+l) > len)
			throw StreamException("Stream - premature end of write()");
		memcpy(&buf[pos],p,l);
		pos += l; 
    }

    virtual bool eof()
    {
        return pos >= len;
    }

	virtual void rewind()
	{
		pos = 0;
	}

	virtual void seekTo(int p)
	{
		pos = p;
	}

	virtual int getPosition()
	{
		return pos;
	}

	void	convertFromBase64();


	char *buf;
	int len,pos;
};