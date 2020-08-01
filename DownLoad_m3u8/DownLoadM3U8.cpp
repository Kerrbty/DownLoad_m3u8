#include "DownLoadM3U8.h"
#include <COneLine/COneLine.h>
#include <list.h>
#include "Log.h"
#include <Http/Http.h>
#include "DecryptBuf.h"
#include "PV/lock.h"
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#ifdef _DEBUG
#pragma comment(lib, "http_d.lib")
#pragma comment(lib, "pv_d.lib")
#pragma comment(lib, "COneLine_d.lib")
#else
#pragma comment(lib, "http.lib")
#pragma comment(lib, "pv.lib")
#pragma comment(lib, "COneLine.lib")
#endif


#define URL_LENTH   1024
#ifdef _DEBUG 
#define MAX_DWONLOAD_THREAD  1
#else
#define MAX_DWONLOAD_THREAD  18
#endif


enum ENCRYPT_TYPE{
    NOT_ENCRYPT = 0,
    ENCRYPT_AES128,
};

typedef struct _DownTsList{
    LIST_ENTRY next;
    DWORD nId;          // 在视频文件中的id序号 
    DWORD nDecodeErrCount;  // 解密失败次数，超过3次就不解密 
    BOOL  bEncrypt;     // 判断是否加密 
    PBYTE pThisFileBuf; // 下载该视频片段的内存 
    DWORD dwFileSize;   // 下载下来视频片段字节数 
    char* chUrl;        // 视频片段Url 
}DownTsList, *PDownTsList; 

static HANDLE hSaveFile = INVALID_HANDLE_VALUE;
volatile DWORD global_dwCountId = 0;

// 下载列表 
static DownTsList TsNeedDownList;
static CLock TsNeedDownListLock;

// 保持文件列表 
static DownTsList TsNeedSaveList;
static CLock TsNeedSaveListLock;

static ENCRYPT_TYPE EncryptType = NOT_ENCRYPT;
static PBYTE lpM3U8Key = NULL;

// 需要略过的几个帧 
static DWORD global_skipStart = 0;
static DWORD global_skipCount = 0;

// 下载数据 
static LPBYTE WINAPI HttpGetData(LPCSTR lpUrl, DWORD* dwDataSize)
{
    LPBYTE bRecvData = NULL;
    if (dwDataSize)
    {
        *dwDataSize = 0;
    }
    
    // 下载三次机会 
    for (int i=0; i<3; i++)
    {
        // 设置请求头信息 
        CHttp HttpData(lpUrl);
        HttpData.SetAcceptA("*/*");
        HttpData.SetAcceptEncodingA("gzip, deflate");
        HttpData.SetAutoUnzip(TRUE);
        HttpData.SetAcceptLanguageA("zh-CN,zh;q=0.9,en;q=0.8");
        HttpData.SetUserAgentA("Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.116 Safari/537.36");

        // 获取数据大小 
        DWORD dwAllocLen = 0;
        LPCSTR lpDataLen = HttpData.GetDataLenthA();
        if (lpDataLen)
        {
            dwAllocLen = strtoul(lpDataLen, NULL, 10);
        }
        if (dwAllocLen == 0)
        {
            dwAllocLen = 10*1024*1024;
        }

        // 申请内存，下载数据 
        bRecvData = (LPBYTE)AllocMemory(dwAllocLen);
        if (bRecvData)
        {
            DWORD dwRecvLen = HttpData.GetData(bRecvData, dwAllocLen, TRUE);
            ASSERT_HEAP(bRecvData);
            if (dwRecvLen > 0)
            {
                // 数据没那么大，重新调整内存 
                if (dwRecvLen < dwAllocLen)
                {
                    bRecvData = (LPBYTE)ReAllocMemory(bRecvData, dwRecvLen);
                    ASSERT_HEAP(bRecvData);
                }
                
                // 成功获取到数据 
                if (dwDataSize)
                {
                    *dwDataSize = dwRecvLen;
                }
                break;
            }
            FreeMemory(bRecvData);
            bRecvData = NULL;
        }
    }
    return bRecvData;
}

static DWORD WINAPI SaveTsThread(LPVOID lParam)
{
    DWORD dwBytes = 0;
    DWORD dwSaveCurrentId = 0;
    TsNeedSaveList.dwFileSize = 0;
    Sleep(1000);
    while(TRUE)
    {
        TsNeedSaveListLock.lock();
        // 判断第一个最小节点块是否是下一个保存的id 
        PDownTsList pInserList = CONTAINING_RECORD(TsNeedSaveList.next.Flink, DownTsList, next);
        if (pInserList->nId == dwSaveCurrentId)
        {
            _RemoveHeadList(&TsNeedSaveList.next);
            TsNeedSaveListLock.unlock();

#pragma region  Exit_FLags
            // 已经结束了 
            if (strcmp( pInserList->chUrl, "EndBlock") == 0)
            {
                FreeMemory(pInserList->pThisFileBuf);
                FreeMemory(pInserList->chUrl);
                FreeMemory(pInserList);
                Sleep(400); // 等待一会儿，等其他下载线程退出 

                // 这里删除列表所有数据鲁 
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

                printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b下载完成\t\t\t\n");
                ExitThread(0);
            }
#pragma endregion Exit_FLags

            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b下载进度: %u/%u\t", dwSaveCurrentId+1, global_dwCountId>MAX_DWONLOAD_THREAD?global_dwCountId-MAX_DWONLOAD_THREAD:global_dwCountId);
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
        Sleep(60); // Sleep一下不要跟加入列表的线程抢锁 
    }
    return 0;
}

static DWORD WINAPI DownTsThread(LPVOID lParam)
{
    // 刚开始列表可能是空的，等待一会儿 
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
                // 如果是空列表，加入首部尾部是一样的 
                // 如果不为空，需要判断先后顺序，按序插入 
                PLIST_ENTRY Currentlink = TsNeedSaveList.next.Flink; // 假设这个是比 PDownOneList->nId 大的数据 
                for ( ;Currentlink != &TsNeedSaveList.next; Currentlink = Currentlink->Flink )
                {
                    PDownTsList pInserList = CONTAINING_RECORD(Currentlink, DownTsList, next);
                    if (pInserList->nId > pDwonOneList->nId)
                    {
                        // 已经没有需要下载的块了，退出循环 
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
            // 不略过下载的，需要正常下载 
            if(!bDownSuccess)
            {
                pDwonOneList->pThisFileBuf = HttpGetData(pDwonOneList->chUrl, &pDwonOneList->dwFileSize);
                ASSERT_HEAP(pDwonOneList->pThisFileBuf);

                if (pDwonOneList->pThisFileBuf != NULL && pDwonOneList->dwFileSize != 0)
                {
                    bDownSuccess = TRUE;
                    do 
                    {
                        // 判断加密方式并解密 
                        if (pDwonOneList->bEncrypt && pDwonOneList->nDecodeErrCount <= 3)
                        {
                            if (EncryptType == ENCRYPT_AES128)
                            {
                                BYTE iv = 128;
                                if( !AESDecrypt(lpM3U8Key, &iv, pDwonOneList->pThisFileBuf, pDwonOneList->dwFileSize) )
                                {
                                    pDwonOneList->nDecodeErrCount++;
                                    logger("AES解密失败(%u)： %s\n", pDwonOneList->nId , pDwonOneList->chUrl);
                                    bDownSuccess = FALSE;
                                    break;
                                }
                            }
                        }
                    } while (FALSE);
                }
            }

            // 下载成功，放入保存队列 
            if (bDownSuccess)
            {
                logger("Download Id: %u\n", pDwonOneList->nId);
                TsNeedSaveListLock.lock();
                // 如果是空列表，加入首部尾部是一样的 
                // 如果不为空，需要判断先后顺序，按序插入 
                PLIST_ENTRY Currentlink = TsNeedSaveList.next.Flink; // 假设这个是比 PDownOneList->nId 大的数据 
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
                // 有问题，还给原下载列表 
                pDwonOneList->dwFileSize = 0;
                if (pDwonOneList->pThisFileBuf)
                {
                    FreeMemory(pDwonOneList->pThisFileBuf);
                }
                logger("下载失败(%u)： %s\n", pDwonOneList->nId, pDwonOneList->chUrl);
                TsNeedDownListLock.lock();
                _InsertHeadList(&TsNeedDownList.next, &pDwonOneList->next);
                TsNeedDownListLock.unlock();
            }
        }
    }
    return 0;
}


static const char* GetFileUrl(LPSTR lpSaveAddr, LPCSTR host_url, char* sub_url)
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


static BOOL AnalyzeM3u8File(LPCSTR lpM3u8Uri, BOOL isURL = TRUE)
{
    // 下载文件 
    BOOL bEncryptFlags = FALSE;
    DWORD dwM3u8Size = 0;
    LPBYTE lpM3u8File = NULL;
    if (isURL)
    {
        dwM3u8Size= 0;
        lpM3u8File = HttpGetData(lpM3u8Uri, &dwM3u8Size);
        ASSERT_HEAP(lpM3u8File);
    }
    else
    {
        DWORD dwBytes = 0;
        HANDLE hFile = FileOpenA(lpM3u8Uri, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            dwM3u8Size = GetFileSize(hFile, NULL);
            lpM3u8File = (LPBYTE)AllocMemory(dwM3u8Size+1);
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            ReadFile(hFile, lpM3u8File, dwM3u8Size, &dwBytes, NULL);
            ASSERT_HEAP(lpM3u8File);
            DeleteHandle(hFile);
        }
    }
    if ( lpM3u8File )
    {
        DWORD dwUrlLen = 0;
        LPSTR lpDownAddress = (LPSTR)AllocMemory(URL_LENTH*2);
        LPSTR lpOneAddr = (LPSTR)AllocMemory(URL_LENTH);
        CReadLine m3u8(lpM3u8File, dwM3u8Size, FALSE);

        dwUrlLen = URL_LENTH;
        while( m3u8.getLine(lpOneAddr, &dwUrlLen) )
        {
            ASSERT_HEAP(lpOneAddr);
            int firstch = 0;
            while(lpOneAddr[firstch] == ' ' || lpOneAddr[firstch] == '\t')
            {
                firstch++;
            }
            if (lpOneAddr[firstch] == '#')
            {
                // 有可能有加密的哦 
                if ( StrStrIA(lpOneAddr, "EXT-X-KEY") != NULL && StrStrIA(lpOneAddr, "AES-128") != NULL ) // 加密 
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
                        char* lpEndURI = StrStrIA(lpKeyURI, "\"");
                        if (lpEndURI != NULL)
                        {
                            *lpEndURI++ = '\0';
                        }

                        const char* lpNewUrl = GetFileUrl(lpDownAddress, lpM3u8Uri, lpKeyURI);
                        ASSERT_HEAP(lpDownAddress);

                        DWORD dwDataLen = 0;
                        LPBYTE lpM3u8File = HttpGetData(lpNewUrl, &dwDataLen);
                        ASSERT_HEAP(lpM3u8File);
                        if (lpM3u8File != NULL)
                        {
                            lpM3U8Key = lpM3u8File;
                            bEncryptFlags = TRUE;
                        }
                    }
                }
                dwUrlLen = URL_LENTH; 
                continue; // 注释的行 
            }

            LPCSTR lpFileExt = strchr(lpOneAddr, '.');
            if (lpFileExt)
            {
                if (StrStrIA(lpFileExt, ".m3u8") != NULL) // m3u8 
                {
                    AnalyzeM3u8File(GetFileUrl(lpDownAddress, lpM3u8Uri, lpOneAddr+firstch), TRUE);
                }
                else if ( StrStrIA(lpFileExt, ".ts") != NULL || StrStrIA(lpFileExt, ".mp4") != NULL )  // 知乎里面ts文件带 auth_key= 的
                {
                    const char* lpNewUrl = GetFileUrl(lpDownAddress, lpM3u8Uri, lpOneAddr+firstch);

                    // 加入列表进行下载 
                    PDownTsList plist = (PDownTsList)AllocMemory(sizeof(DownTsList));
                    ASSERT_HEAP(plist);
                    plist->nId = global_dwCountId++;
                    plist->nDecodeErrCount = 0;
                    plist->bEncrypt = bEncryptFlags;
                    plist->chUrl = (char*)AllocMemory(strlen(lpNewUrl)+1);
                    strcpy(plist->chUrl, lpNewUrl);
                    ASSERT_HEAP(plist->chUrl);
                    TsNeedDownListLock.lock();
                    _InsertTailList(&TsNeedDownList.next, &plist->next);
                    TsNeedDownListLock.unlock();
                }
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


BOOL DownM3u8(LPCSTR lpM3u8Url, BOOL isFromHttp, LPCSTR lpSaveFile, DWORD dwSkipStart, DWORD dwSkipCount)
{
    if (lpM3u8Url==NULL && lpSaveFile==NULL )
    {
        return FALSE;
    }

    global_skipStart = dwSkipStart;
    global_skipCount = dwSkipCount;

    // 初始化列表 
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
        printf("正在初始化...");
        AnalyzeM3u8File(lpM3u8Url, isFromHttp);  // 分析M3U8文件  


        for (i=0; i<MAX_DWONLOAD_THREAD; i++)
        {
            // 最后加入 MAX_DWONLOAD_THREAD 个数据块，结束标志,每个线程消耗一个 
            PDownTsList plist = (PDownTsList)AllocMemory(sizeof(DownTsList));
            plist->nId = global_dwCountId++;
            plist->chUrl = (char*)AllocMemory(MAX_PATH);
            strcpy(plist->chUrl, "EndBlock");
            ASSERT_HEAP(plist);
            ASSERT_HEAP(plist->chUrl);
            TsNeedDownListLock.lock();
            _InsertTailList(&TsNeedDownList.next, &plist->next);
            TsNeedDownListLock.unlock();
        }

        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b正在下载...");
        // 等待结束 
        WaitForMultipleObjects(MAX_DWONLOAD_THREAD, hThread, TRUE, INFINITE);
        for (i=0; i<MAX_DWONLOAD_THREAD; i++)
        {
            DeleteHandle(hThread[i]);
        }
        if (lpM3U8Key)
        {
            FreeMemory(lpM3U8Key);
        }

        // 等待保持完整的Ts文件再关闭文件结束 
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

        printf("耗时: ");
        if (dwHours != 0)
        {
            printf("%u小时 ", dwHours);
        }
        if (dwHours != 0 || dwMinutes != 0)
        {
            printf("%u分钟 ", dwMinutes);
        }
        printf("%u秒 %u, ", dwSeconds, dwMilliSeconds);

        // 下载平局速度 TsNeedSaveList.dwFileSize 
        int ich = 0;
        for (ich=0; ich<4 && dwDownSpeed>1024.0; ich++)
        {
            dwDownSpeed /= 1024.0;
        }
        printf("平均下载速度: %0.2f%cB/S\n", dwDownSpeed, " KMGT"[ich]);
        return TRUE;
    }
    return FALSE;
}