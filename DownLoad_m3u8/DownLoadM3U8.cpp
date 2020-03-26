#include "DownLoadM3U8.h"
#include "ReadOneLine.h"
#include "List.h"
#include "Log.h"
#include "Http/HttpPost.h"
#include "DecryptBuf.h"
#include "PV/lock.h"
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#define URL_LENTH   1024
#define MAX_DWONLOAD_THREAD  18

enum ENCRYPT_TYPE{
    NOT_ENCRYPT = 0,
    ENCRYPT_AES128,
};

typedef struct _DownTsList{
    LIST_ENTRY next;
    DWORD nId;          // ����Ƶ�ļ��е�id��� 
    DWORD nDecodeErrCount;  // ����ʧ�ܴ���������3�ξͲ����� 
    BOOL  bEncrypt;     // �ж��Ƿ���� 
    PBYTE pThisFileBuf; // ���ظ���ƵƬ�ε��ڴ� 
    DWORD dwFileSize;   // ����������ƵƬ���ֽ��� 
    char* chUrl;        // ��ƵƬ��Url 
}DownTsList, *PDownTsList; 

static HANDLE hSaveFile = INVALID_HANDLE_VALUE;
volatile DWORD global_dwCountId = 0;

// �����б� 
static DownTsList TsNeedDownList;
static CLock TsNeedDownListLock;

// �����ļ��б� 
static DownTsList TsNeedSaveList;
static CLock TsNeedSaveListLock;

static ENCRYPT_TYPE EncryptType = NOT_ENCRYPT;
static PBYTE lpM3U8Key = NULL;

// ��Ҫ�Թ��ļ���֡ 
static DWORD global_skipStart = 0;
static DWORD global_skipCount = 0;

static DWORD WINAPI SaveTsThread(LPVOID lParam)
{
    DWORD dwBytes = 0;
    DWORD dwSaveCurrentId = 0;
    TsNeedSaveList.dwFileSize = 0;
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
                Sleep(400); // �ȴ�һ����������������߳��˳� 

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

            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b���ؽ���: %u/%u\t", dwSaveCurrentId+1, global_dwCountId>MAX_DWONLOAD_THREAD?global_dwCountId-MAX_DWONLOAD_THREAD:global_dwCountId);
            logger("Save Id: %u\n", dwSaveCurrentId);
            if (pInserList->pThisFileBuf != NULL && pInserList->dwFileSize>0)
            {
                WriteFile(hSaveFile, pInserList->pThisFileBuf, pInserList->dwFileSize, &dwBytes, NULL);
                TsNeedSaveList.dwFileSize += pInserList->dwFileSize;
            }
            
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


static DWORD WINAPI DownTsThread(LPVOID lParam)
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
            BOOL bDownSuccess = FALSE;

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
            else if (global_skipCount > 0)
            {
                if (pDwonOneList->nId>=global_skipStart && pDwonOneList->nId<global_skipStart+global_skipCount)
                {
                    pDwonOneList->pThisFileBuf = NULL;
                    pDwonOneList->dwFileSize = 0;
                    logger("Skip Download Id: %u\n", pDwonOneList->nId);
                    bDownSuccess = TRUE;
                }
            }
            // ���Թ����صģ���Ҫ�������� 
            if(!bDownSuccess)
            {
                pDwonOneList->pThisFileBuf = GetHttpDataA(pDwonOneList->chUrl, &pDwonOneList->dwFileSize);
                if (pDwonOneList->pThisFileBuf != NULL && pDwonOneList->dwFileSize != 0)
                {
                    bDownSuccess = TRUE;
                    do 
                    {
                        // �жϼ��ܷ�ʽ������ 
                        if (pDwonOneList->bEncrypt && pDwonOneList->nDecodeErrCount <= 3)
                        {
                            if (EncryptType == ENCRYPT_AES128)
                            {
                                BYTE iv = 128;
                                if( !AESDecrypt(lpM3U8Key, &iv, pDwonOneList->pThisFileBuf, pDwonOneList->dwFileSize) )
                                {
                                    pDwonOneList->nDecodeErrCount++;
                                    logger("AES����ʧ�ܣ� %s\n", pDwonOneList->chUrl);
                                    bDownSuccess = FALSE;
                                    break;
                                }
                            }
                        }
                    } while (FALSE);
                }
            }

            // ���سɹ������뱣����� 
            if (bDownSuccess)
            {
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


static const char* GetFileUrl(char* lpSaveAddr, LPCSTR host_url, char* sub_url)
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


static BOOL AnalyzeM3u8File(LPCSTR lpM3u8Url)
{
    // �����ļ� 
    BOOL bEncryptFlags = FALSE;
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
                    EncryptType = ENCRYPT_AES128;
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
                            bEncryptFlags = TRUE;
                        }
                    }
                }
                dwUrlLen = URL_LENTH; 
                continue; // ע�͵��� 
            }

            LPCSTR lpFileExt = strchr(lpOneAddr, '.');
            if (stricmp(lpFileExt, ".m3u8") == 0) // m3u8 
            {
                AnalyzeM3u8File(GetFileUrl(lpDownAddress, lpM3u8Url, lpOneAddr+firstch));
            }
            else if ( StrStrIA(lpFileExt, ".ts") != NULL || StrStrIA(lpFileExt, ".mp4") != NULL )  // ֪������ts�ļ��� auth_key= ��
            {
                const char* lpNewUrl = GetFileUrl(lpDownAddress, lpM3u8Url, lpOneAddr+firstch);

                // �����б�������� 
                PDownTsList plist = (PDownTsList)AllocMemory(sizeof(DownTsList));
                plist->nId = global_dwCountId++;
                plist->nDecodeErrCount = 0;
                plist->bEncrypt = bEncryptFlags;
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


BOOL DownM3u8(LPCSTR lpM3u8Url, LPCSTR lpSaveFile, DWORD dwSkipStart, DWORD dwSkipCount)
{
    if (lpM3u8Url==NULL && lpSaveFile==NULL )
    {
        return FALSE;
    }

    global_skipStart = dwSkipStart;
    global_skipCount = dwSkipCount;

    // ��ʼ���б� 
    _InitializeListHead(&TsNeedDownList.next);
    _InitializeListHead(&TsNeedSaveList.next);
    TsNeedDownList.nId = -1;
    TsNeedSaveList.nId = -1;
    global_dwCountId = 0;

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

        DWORD dwStartTime = GetTickCount();
        printf("���ڳ�ʼ��...");
        AnalyzeM3u8File(lpM3u8Url);  // ����M3U8�ļ�  


        for (i=0; i<MAX_DWONLOAD_THREAD; i++)
        {
            // ������ MAX_DWONLOAD_THREAD �����ݿ飬������־,ÿ���߳�����һ�� 
            PDownTsList plist = (PDownTsList)AllocMemory(sizeof(DownTsList));
            plist->nId = global_dwCountId++;
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

        DWORD dwEndTime = GetTickCount();
        DWORD dwMilliSeconds = dwEndTime - dwStartTime;
        DWORD dwSeconds = dwMilliSeconds/1000;
        dwMilliSeconds = dwMilliSeconds%1000;

        float dwDownSpeed = 0;
        if (dwSeconds)
        {
            dwDownSpeed = ((float)TsNeedSaveList.dwFileSize)/dwSeconds;
        }

        DWORD dwMinutes = dwSeconds/60;
        dwSeconds = dwSeconds%60;
        DWORD dwHours = dwMinutes/60;
        dwMinutes = dwMinutes%60;

        printf("��ʱ: ");
        if (dwHours != 0)
        {
            printf("%uСʱ ", dwHours);
        }
        if (dwHours != 0 || dwMinutes != 0)
        {
            printf("%u���� ", dwMinutes);
        }
        printf("%u�� %u, ", dwSeconds, dwMilliSeconds);

        // ����ƽ���ٶ� TsNeedSaveList.dwFileSize 
        int ich = 0;
        for (ich=0; ich<4 && dwDownSpeed>1024.0; ich++)
        {
            dwDownSpeed /= 1024.0;
        }
        printf("ƽ�������ٶ�: %0.2f%cB/S\n", dwDownSpeed, " KMGT"[ich]);
        return TRUE;
    }
    return FALSE;
}