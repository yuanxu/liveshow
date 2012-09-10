#pragma once

class Segment;
class IDeliver
{
public:
	virtual void Deliver(Segment*,bool)=0;
	virtual void DeliverHeader(char *,int)=0;
	virtual void ImmediateDeliver(Segment*,bool)=0;
	//期待的下一个SegmentID
	DWORD dwExpectSegmentID;
	bool bDeliveredHeader;
};
