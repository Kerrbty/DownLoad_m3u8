#ifndef _DECRYPTBUF_MODULE_20190413_0109_HEADER_
#define _DECRYPTBUF_MODULE_20190413_0109_HEADER_ 
#include "defs.h"


//////////////////////////////////////////////////////////////////////////
//
// AESDecrypt  AES 解密函数 
//        key  解密秘钥 
//        iv   解密过程中秘钥的长度（不同模式长度不一）
//        inData   输入加密的数据 
//        inSize  输入加密的数据长度 
/////////////////////////////////////////////////////////////////////////
// AES-128/CBC/PKCS5Padding
// AES-256: KeySize=256, IVSize=128, Result=128
BOOL AESDecrypt(PBYTE key, BYTE* iv, PBYTE inData, DWORD inSize);

#endif // _DECRYPTBUF_MODULE_20190413_0109_HEADER_