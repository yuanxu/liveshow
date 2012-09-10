#include "StdAfx.h"
#include "FileDeliver.h"

using namespace std;

//FileDeliver::m_file("1.asf" ,ios::out|ios::binary);
FileDeliver::FileDeliver(void)
{
	lpFile= new ofstream("1.asf" ,ios::out|ios::binary);
}

FileDeliver::~FileDeliver(void)
{
	lpFile->close();
}

void FileDeliver::Deliver(Segment *lpSegment, bool bFlag)
{
	if(!lpSegment||!lpSegment->getHasData())
		return;
	int iSize = Segment::getSegmentSize();
	char *p = NULL;
	lpSegment->getData(&p);
	lpFile->write(p,iSize);
	delete[] p;
}

void FileDeliver::DeliverHeader(char *p,int len)
{
	lpFile->write(p,len);
}