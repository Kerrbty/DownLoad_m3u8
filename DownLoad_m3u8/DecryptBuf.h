#ifndef _DECRYPTBUF_MODULE_20190413_0109_HEADER_
#define _DECRYPTBUF_MODULE_20190413_0109_HEADER_ 
#include "defs.h"


//////////////////////////////////////////////////////////////////////////
//
// AESDecrypt  AES ���ܺ��� 
//        key  ������Կ 
//        iv   ���ܹ�������Կ�ĳ��ȣ���ͬģʽ���Ȳ�һ��
//        inData   ������ܵ����� 
//        inSize  ������ܵ����ݳ��� 
/////////////////////////////////////////////////////////////////////////
// AES-128/CBC/PKCS5Padding
// AES-256: KeySize=256, IVSize=128, Result=128
BOOL AESDecrypt(PBYTE key, BYTE* iv, PBYTE inData, DWORD inSize);

#endif // _DECRYPTBUF_MODULE_20190413_0109_HEADER_