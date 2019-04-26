#include "Http/HttpPost.h"
#include "ReadOneLine.h"
#include "List.h"
#include "PV/lock.h"
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#define URL_LENTH   1024
#define MAX_DWONLOAD_THREAD  18

typedef struct _DownTsList{
    LIST_ENTRY next;
    DWORD nId;          // ����Ƶ�ļ��е�id��� 
    PBYTE pThisFileBuf; // ���ظ���ƵƬ�ε��ڴ� 
    DWORD dwFileSize;   // ����������ƵƬ���ֽ��� 
    char* chUrl;        // ��ƵƬ��Url 
}DownTsList, *PDownTsList; 

HANDLE hSaveFile = INVALID_HANDLE_VALUE;
volatile DWORD dwCountId = 0;

// �����б� 
DownTsList TsNeedDownList;
CLock TsNeedDownListLock;

// �����ļ��б� 
DownTsList TsNeedSaveList;
CLock TsNeedSaveListLock;

// ��־�ļ�
CLock LogFileLock;

PBYTE lpM3U8Key = NULL;


bool create_directory(const char* szpath)
{
    if (szpath == NULL)
    {
        return false;
    }

    size_t len = strlen(szpath);
    char* szdir = (char*)AllocMemory((len+1)*sizeof(char));
    if (szdir != NULL)
    {
        char *p = szdir;
        const char *q = szpath;

        memset(szdir, 0, len+1);
        while(*q!= NULL)
        {
            if (*q == '\\')
            {
                CreateDirectoryA(szdir, NULL);
            }
            *p++ = *q++;
        }
        CreateDirectoryA(szdir, NULL);
        FreeMemory(szdir);
    }
    else
    {
        return false;
    }
    return true;
}

int logger(const char* Format, ...)
{    
    static FILE* file = NULL;
    int rvl = 0;
    va_list va;
    va_start(va, Format);

    SYSTEMTIME SystemTime = {0};
    GetLocalTime(&SystemTime);
    if (file == NULL)
    {

        char* filename = (char*)AllocMemory(1024*sizeof(char));
        GetModuleFileNameA(NULL, filename, 1024);
        size_t szlen = strlen(filename);
        if (szlen > 0)
        {
            szlen--;
            while(filename[szlen] != '\\')
            {
                szlen--;
            }
            strcpy(filename+szlen, "\\Log\\");
        }
        create_directory(filename); 
        szlen = strlen(filename);
        sprintf(filename+szlen, "LogInfo-%04d%02d%02d-%02d%02d.log", SystemTime.wYear,  SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute);
        LogFileLock.lock();
        if (file == NULL)
        {
            file = fopen(filename, "w+");
        }
        LogFileLock.unlock();
        FreeMemory(filename);
    }
    if (file != NULL)
    { 
        LogFileLock.lock();
        fprintf(file, "[%u/%u/%u %02u:%02u:%u][thread_id: %u] ", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
            SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, GetCurrentThreadId());

        rvl = vfprintf(file, Format, va);
        fflush(file);
        LogFileLock.unlock();
    }
    va_end(va);
    return rvl;
}

DWORD WINAPI SaveTsThread(LPVOID lParam)
{
    DWORD dwBytes = 0;
    DWORD dwSaveCurrentId = 0;
    Sleep(1000);
    while(TRUE)
    {
        TsNeedSaveListLock.lock();
		// �жϵ�һ����С�ڵ���Ƿ�����һ�������id 
        PDownTsList pInserList = CONTAINING_RECORD(TsNeedSaveList.next.Flink, DownTsList, next);
        if (pInserList->nId == dwSaveCurrentId)
        {
            _RemoveHeadList(&TsNeedSaveList.next);
            TsNeedSaveListLock.unlock();

#pragma region  Exit_FLags
            // �Ѿ������� 
            if (strcmp( pInserList->chUrl, "EndBlock") == 0)
            {
                FreeMemory(pInserList->pThisFileBuf);
                FreeMemory(pInserList->chUrl);
                FreeMemory(pInserList);
                Sleep(400); // �ȴ�һ��������������������˳� 

                // ����ɾ���б���������³ 
                TsNeedSaveListLock.lock();
                while(!_IsListEmpty(&TsNeedSaveList.next))
                {
                    PLIST_ENTRY Remotelink = _RemoveHeadList(&TsNeedSaveList.next);
                    pInserList = CONTAINING_RECORD(Remotelink, DownTsList, next);
                    FreeMemory(pInserList->pThisFileBuf);
                    FreeMemory(pInserList->chUrl);
                    FreeMemory(pInserList);
                }
                TsNeedSaveListLock.unlock();

                printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b�������\t\t\t\n");
                ExitThread(0);
            }
#pragma endregion Exit_FLags

            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b���ؽ���: %u/%u", dwSaveCurrentId, dwCountId-MAX_DWONLOAD_THREAD);
            logger("Save Id: %u\n", dwSaveCurrentId);
            WriteFile(hSaveFile, pInserList->pThisFileBuf, pInserList->dwFileSize, &dwBytes, NULL);
            FreeMemory(pInserList->pThisFileBuf);
            FreeMemory(pInserList->chUrl);
            FreeMemory(pInserList);

            dwSaveCurrentId++;
        }
        else
        {
            TsNeedSaveListLock.unlock();
            Sleep(100);
        }
        Sleep(60); // Sleepһ�²�Ҫ�������б���߳����� 
    }
    return 0;
}

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
        printf("Get provider context failed!\n");
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
            printf("Create key failed!\n");
            break;
        }

        // ���ó�ʼ����
        if(iv == NULL)
        {
            if(!CryptSetKeyParam(hKey, KP_IV, key, 0))
            {
                printf("Set key's IV parameter failed!\n");
                break;
            }
        }
        else
        {
            if(!CryptSetKeyParam(hKey, KP_IV, iv, 0))
            {
                printf("Set key's IV parameter failed!\n");
                break;
            }
        }

        if(!CryptDecrypt(hKey, NULL, TRUE, 0, inData, &inSize))
        {
            printf("AES encrypt failed!\n");
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

DWORD WINAPI DownTsThread(LPVOID lParam)
{
    // �տ�ʼ�б�����ǿյģ��ȴ�һ��� 
    while( TRUE )
    {
        TsNeedDownListLock.lock();
        if ( _IsListEmpty(&TsNeedDownList.next) )
        {
            TsNeedDownListLock.unlock();
            Sleep(20);
        }
        else
        {
            PLIST_ENTRY pNowList = _RemoveHeadList(&TsNeedDownList.next);
            TsNeedDownListLock.unlock();

            PDownTsList pDwonOneList = CONTAINING_RECORD(pNowList, DownTsList, next);

            if (strcmp(pDwonOneList->chUrl, "EndBlock") == 0)
            {
                TsNeedSaveListLock.lock();
                // ����ǿ��б������ײ�β����һ���� 
                // �����Ϊ�գ���Ҫ�ж��Ⱥ�˳�򣬰������ 
                PLIST_ENTRY Currentlink = TsNeedSaveList.next.Flink; // ��������Ǳ� PDownOneList->nId ������� 
                for ( ;Currentlink != &TsNeedSaveList.next; Currentlink = Currentlink->Flink )
                {
                    PDownTsList pInserList = CONTAINING_RECORD(Currentlink, DownTsList, next);
                    if (pInserList->nId > pDwonOneList->nId)
                    {
						// �Ѿ�û����Ҫ���صĿ��ˣ��˳�ѭ�� 
                        break; 
                    }
                }
                _InsertTailList(Currentlink, &pDwonOneList->next);
                TsNeedSaveListLock.unlock();
                break;
            }

            BOOL bDownSuccess = FALSE;
            pDwonOneList->pThisFileBuf = GetHttpDataA(pDwonOneList->chUrl, &pDwonOneList->dwFileSize);
            if (pDwonOneList->pThisFileBuf != NULL && pDwonOneList->dwFileSize != 0)
            {
                do 
                {
                    if (lpM3U8Key)
                    {
                        // ���� 
                        BYTE iv = 128;
                        if( !AESDecrypt(lpM3U8Key, &iv, pDwonOneList->pThisFileBuf, pDwonOneList->dwFileSize) )
                        {
                            logger("AES����ʧ�ܣ� %s\n", pDwonOneList->chUrl);
                            bDownSuccess = FALSE;
                        }
                    }
                    logger("Download Id: %u\n", pDwonOneList->nId);
                    TsNeedSaveListLock.lock();
                    // ����ǿ��б������ײ�β����һ���� 
                    // �����Ϊ�գ���Ҫ�ж��Ⱥ�˳�򣬰������ 
                    PLIST_ENTRY Currentlink = TsNeedSaveList.next.Flink; // ��������Ǳ� PDownOneList->nId ������� 
                    for ( ;Currentlink != &TsNeedSaveList.next; Currentlink = Currentlink->Flink )
                    {
                        PDownTsList pInserList = CONTAINING_RECORD(Currentlink, DownTsList, next);
                        if (pInserList->nId > pDwonOneList->nId)
                        {
                            break;
                        }
                    }
                    _InsertTailList(Currentlink, &pDwonOneList->next);
                    TsNeedSaveListLock.unlock();
                    bDownSuccess = TRUE;
                } while (FALSE);
            }
            
            if (!bDownSuccess)
            {
                // �����⣬����ԭ�����б� 
                pDwonOneList->dwFileSize = 0;
                if (pDwonOneList->pThisFileBuf)
                {
                    FreeMemory(pDwonOneList->pThisFileBuf);
                }
                logger("����ʧ�ܣ� %s\n", pDwonOneList->chUrl);
                TsNeedDownListLock.lock();
                _InsertHeadList(&TsNeedDownList.next, &pDwonOneList->next);
                TsNeedDownListLock.unlock();
            }
        }
    }
    return 0;
}


const char* GetFileUrl(char* lpSaveAddr, LPCSTR host_url, char* sub_url)
{
    // downfile and analyze 
    if (memicmp(sub_url, "http://", 7) == 0 || memicmp(sub_url, "https://", 8) == 0)
    {
        strcpy(lpSaveAddr, sub_url);
    }
    else if ( *sub_url == '/' )
    {
        strcpy(lpSaveAddr, host_url);
        char* lpLastM3u8Name = (char*)strchr(lpSaveAddr+8, '/');
        strcpy(lpLastM3u8Name, sub_url);
    }
    else
    {
        strcpy(lpSaveAddr, host_url);
        char* lpLastM3u8Name = (char*)strrchr(lpSaveAddr, '/');
        strcpy(lpLastM3u8Name+1, sub_url);
    }

    return lpSaveAddr;
}


BOOL AnalyzeM3u8File(LPCSTR lpM3u8Url)
{
    // �����ļ� 
    DWORD dwM3u8Size = 0;
    LPBYTE lpM3u8File = GetHttpDataA(lpM3u8Url, &dwM3u8Size);
    if ( lpM3u8File )
    {
        DWORD dwUrlLen = 0;
        LPSTR lpDownAddress = (LPSTR)AllocMemory(URL_LENTH*2);
        LPSTR lpOneAddr = (LPSTR)AllocMemory(URL_LENTH);
        CReadLine m3u8(lpM3u8File, dwM3u8Size, FALSE);

        dwUrlLen = URL_LENTH;
        while( m3u8.getLine(lpOneAddr, &dwUrlLen) )
        {
            int firstch = 0;
            while(lpOneAddr[firstch] == ' ' || lpOneAddr[firstch] == '\t')
            {
                firstch++;
            }
            if (lpOneAddr[firstch] == '#')
            {
                // �п����м��ܵ�Ŷ 
                if ( StrStrIA(lpOneAddr, "EXT-X-KEY") != NULL && StrStrIA(lpOneAddr, "AES-128") != NULL ) // ���� 
                {  
                    char* lpKeyURI = StrStrIA(lpOneAddr, "URI=\"");
                    if (lpKeyURI != NULL)
                    {
                        lpKeyURI = lpKeyURI+4;
                        if (lpKeyURI[0] == '\"')
                        {
                            lpKeyURI++;
                        }
                        int len = strlen(lpKeyURI);
                        if (len >0 && lpKeyURI[len-1] == '\"')
                        {
                            lpKeyURI[len-1] = '\0';
                        }
                        const char* lpNewUrl = GetFileUrl(lpDownAddress, lpM3u8Url, lpKeyURI);

                        LPBYTE lpM3u8File = GetHttpDataA(lpNewUrl, &dwM3u8Size);
                        if (lpM3u8File != NULL)
                        {
                            lpM3U8Key = lpM3u8File;
                        }
                    }
                }
                dwUrlLen = URL_LENTH; 
                continue; // ע�͵��� 
            }

            LPCSTR lpFileExt = PathFindExtensionA(lpOneAddr);
            if (stricmp(lpFileExt, ".m3u8") == 0) // m3u8 
            {
                AnalyzeM3u8File(GetFileUrl(lpDownAddress, lpM3u8Url, lpOneAddr+firstch));
            }
            else if ( StrStrIA(lpFileExt, ".ts") != NULL )  // ֪������ts�ļ��� auth_key= ��
            {
                const char* lpNewUrl = GetFileUrl(lpDownAddress, lpM3u8Url, lpOneAddr+firstch);

                // �����б�������� 
                PDownTsList plist = (PDownTsList)AllocMemory(sizeof(DownTsList));
                plist->nId = dwCountId++;
                plist->chUrl = (char*)AllocMemory(strlen(lpNewUrl)+1);
                strcpy(plist->chUrl, lpNewUrl);
                TsNeedDownListLock.lock();
                _InsertTailList(&TsNeedDownList.next, &plist->next);
                TsNeedDownListLock.unlock();
            }

            dwUrlLen = URL_LENTH;
        }

        FreeMemory(lpOneAddr);
        FreeMemory(lpDownAddress);
        FreeMemory(lpM3u8File);
        return TRUE;
    }
    return FALSE;
}


BOOL DownM3u8(LPCSTR lpM3u8Url, LPCSTR lpSaveFile)
{
    if (lpM3u8Url==NULL && lpSaveFile==NULL )
    {
        return FALSE;
    }

    // ��ʼ���б� 
    _InitializeListHead(&TsNeedDownList.next);
    _InitializeListHead(&TsNeedSaveList.next);
    TsNeedDownList.nId = -1;
    TsNeedSaveList.nId = -1;
    dwCountId = 0;

    hSaveFile = FileOpenA(lpSaveFile, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS);
    if (hSaveFile != INVALID_HANDLE_VALUE)
    {
        int i=0;
        HANDLE hThread[MAX_DWONLOAD_THREAD]; 
        for (i=0; i<MAX_DWONLOAD_THREAD; i++)
        {
            hThread[i] = CreateThread(NULL, 0, DownTsThread, NULL, 0, NULL);
        }
        HANDLE hSaveAllFile = CreateThread(NULL, 0, SaveTsThread, NULL, 0, NULL);

        printf("���ڳ�ʼ��...");
        AnalyzeM3u8File(lpM3u8Url);  // ����M3U8�ļ�  


        for (i=0; i<MAX_DWONLOAD_THREAD; i++)
        {
            // ������ MAX_DWONLOAD_THREAD �����ݿ飬������־,ÿ���߳�����һ�� 
            PDownTsList plist = (PDownTsList)AllocMemory(sizeof(DownTsList));
            plist->nId = dwCountId++;
            plist->chUrl = (char*)AllocMemory(MAX_PATH);
            strcpy(plist->chUrl, "EndBlock");
            TsNeedDownListLock.lock();
            _InsertTailList(&TsNeedDownList.next, &plist->next);
            TsNeedDownListLock.unlock();
        }

        // �ȴ����� 
        WaitForMultipleObjects(MAX_DWONLOAD_THREAD, hThread, TRUE, INFINITE);
        for (i=0; i<MAX_DWONLOAD_THREAD; i++)
        {
            DeleteHandle(hThread[i]);
        }
        if (lpM3U8Key)
        {
            FreeMemory(lpM3U8Key);
        }

        // �ȴ�����������Ts�ļ��ٹر��ļ����� 
        WaitForSingleObject(hSaveAllFile, INFINITE);
        DeleteHandle(hSaveAllFile);
        DeleteHandle(hSaveFile);
        return TRUE;
    }
    return FALSE;
}


void usage(const char* selfname)
{
    const char* lpname = PathFindFileNameA(selfname);
    printf("M3U8���ع��� -- v1.0.0\n\n");
    printf("%s /U M3U8 /F SaveFile  \n", lpname);
    printf("  /U  M3U8      M3U8�ļ���URL\n");
    printf("  /F SaveFile   ��������Ts�ļ���\n\n");
}

int main(int argc, char* argv[], char* env[])
{
#ifndef _DEBUG
    if (argc < 2)
    {
        usage(argv[0]);
        return -1;
    }
    LPCSTR lpM3u8Url = NULL;
    LPCSTR lpSaveFile = NULL;

    for (int i=1; i<argc-1; i++)
    {
        if ( memicmp(argv[i], "/U", 2 ) == 0 ||
            memicmp(argv[i], "-U", 2 ) == 0 )
        {
            lpM3u8Url = argv[++i];
        }
        else if (memicmp(argv[i], "/F", 2 ) == 0 ||
            memicmp(argv[i], "-F", 2 ) == 0 )
        {
            lpSaveFile = argv[++i];
        }
    }
    DownM3u8(lpM3u8Url, lpSaveFile);
#else
    DownM3u8("http://cdn.luya9.com/ppvod/512DEC0E4B55C857E1FFE628B6CD0401.m3u8", "F:\\av.ts");
#endif
    return 0;
}