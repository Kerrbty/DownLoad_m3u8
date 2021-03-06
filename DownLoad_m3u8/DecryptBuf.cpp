#include "DecryptBuf.h"

// AES-128/CBC/PKCS5Padding
// AES-256: KeySize=256, IVSize=128, Result=128
BOOL AESDecrypt(PBYTE key, BYTE* iv, PBYTE inData, DWORD inSize)
{
    BOOL bDecryptSuccess = FALSE;

    static BOOL bInitCrypt = FALSE;
    static HCRYPTPROV hProv = NULL; 
    static HCRYPTKEY hKey = NULL;
    if (!bInitCrypt)
    {
        if(!CryptAcquireContext(
            &hProv,                // 返回的句柄
            NULL,                // CSP key 容器名称
            NULL,                // CSP 提供者名称
            PROV_RSA_AES,        // CSP 提供者类型
            CRYPT_VERIFYCONTEXT)) // 附加参数：
        {
            return bDecryptSuccess;
        }

        // 创建 Key
        struct keyBlob
        {
            BLOBHEADER hdr;
            DWORD cbKeySize;
            BYTE rgbKeyData[16];                // FOR AES-256 = 32
        } keyBlob;

        keyBlob.hdr.bType = PLAINTEXTKEYBLOB;
        keyBlob.hdr.bVersion = CUR_BLOB_VERSION;
        keyBlob.hdr.reserved = 0;
        keyBlob.hdr.aiKeyAlg = CALG_AES_128;    // FOR AES-256 = CALG_AES_256
        keyBlob.cbKeySize = 16;                    // FOR AES-256 = 32
        CopyMemory(keyBlob.rgbKeyData, key, keyBlob.cbKeySize);

        do 
        {
            if (!CryptImportKey(hProv, (BYTE*)(&keyBlob), sizeof(keyBlob), NULL, CRYPT_EXPORTABLE, &hKey))
            {
                break;
            }

            // 设置初始向量
            if(iv == NULL)
            {
                if(!CryptSetKeyParam(hKey, KP_IV, key, 0))
                {
                    break;
                }
            }
            else
            {
                if(!CryptSetKeyParam(hKey, KP_IV, iv, 0))
                {
                    break;
                }
            }
            bInitCrypt = TRUE;
        } while (false);
    }

    if ( CryptDecrypt(hKey, NULL, TRUE, 0, inData, &inSize) )
    {
        bDecryptSuccess = TRUE;
    }
    return bDecryptSuccess;
}