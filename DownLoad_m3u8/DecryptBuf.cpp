#include "DecryptBuf.h"

// AES-128/CBC/PKCS5Padding
// AES-256: KeySize=256, IVSize=128, Result=128
BOOL AESDecrypt(PBYTE key, BYTE* iv, PBYTE inData, DWORD inSize)
{
    BOOL bDecryptSuccess = FALSE;
    HCRYPTPROV hProv = NULL; 
    if(!CryptAcquireContext(
        &hProv,                // ���صľ��
        NULL,                // CSP key ��������
        NULL,                // CSP �ṩ������
        PROV_RSA_AES,        // CSP �ṩ������
        0))            // ���Ӳ�����
    {
        return bDecryptSuccess;
    }

    // ���� Key
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

    HCRYPTKEY hKey = NULL;
    do 
    {
        if (!CryptImportKey(hProv, (BYTE*)(&keyBlob), sizeof(keyBlob), NULL, CRYPT_EXPORTABLE, &hKey))
        {
            break;
        }

        // ���ó�ʼ����
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

        if(!CryptDecrypt(hKey, NULL, TRUE, 0, inData, &inSize))
        {
            break;
        }
        bDecryptSuccess = TRUE;
    } while (false);

    if(hKey != NULL)
    {
        CryptDestroyKey(hKey);
    }

    if(hProv != NULL)
    {
        CryptReleaseContext(hProv, 0);
    }
    return bDecryptSuccess;
}