#pragma once
#include "ideliver.h"
#include "Segment.h"

using namespace std;
class FileDeliver :
	public IDeliver
{
private:
	 ofstream * lpFile;
public:
	FileDeliver(void);
	~FileDeliver(void);

	void Deliver(Segment*,bool);
	void DeliverHeader(char*,int);
};
