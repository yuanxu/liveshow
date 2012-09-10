#include "StdAfx.h"
#include "asf.h"


DWORD asfHeader::getChunkSize()
{
	DWORD ret =0;
	memcpy(&ret,buf + find(asf_filePropObjID) + HDR_ASF_CHUNKLENGTH_4,4);
	return ret;
}

DWORD asfHeader::getBitRate()
{
	DWORD ret =0;
	memcpy(&ret,buf + find(asf_filePropObjID) + HDR_ASF_BITRATE,4);
	return ret;
}