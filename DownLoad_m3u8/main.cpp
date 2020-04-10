#include <stdio.h>
#include <Shlwapi.h>
#include <assert.h>
#include "DownLoadM3U8.h"

static LPCSTR GetSaveName()
{
    LPSTR lpFileName = (LPSTR)AllocMemory(MAX_PATH*2*sizeof(char));
    GetModuleFileNameA(GetModuleHandle(NULL), lpFileName, MAX_PATH);
    PathRemoveFileSpecA(lpFileName);
    int len = strlen(lpFileName);
    int i = 0;
    do 
    {
        wsprintfA(lpFileName+len, "\\temp%0d.ts", i);
        i++;
    } while (PathFileExistsA(lpFileName));
    return lpFileName;
}

static void GetSkip(LPSTR lpSkipStr, DWORD *nSkipStart, DWORD *nSkipCount)
{
    assert(lpSkipStr);
    assert(nSkipStart);
    assert(nSkipCount);
    char* pNextStr = NULL;
    *nSkipStart = strtoul(lpSkipStr, &pNextStr, 10);
    if (*pNextStr == '-')
    {
        *nSkipCount = strtoul(pNextStr+1, NULL, 10);
    }
    else
    {
        *nSkipCount = 1;
    }
}

static void usage(const char* selfname)
{
    const char* lpname = PathFindFileNameA(selfname);
    printf("M3U8下载工具 -- v1.0.0\n\n");
    printf("%s [/U M3U8_URL | /R M3U8_File] /F SaveFile  \n", lpname);
    printf("  /U  M3U8 URL    M3U8文件的URL\n");
    printf("  /R  M3U8 File   设置本地的M3U8文件\n");
    printf("  /Skip StartId-nCount   略过从某一个帧开始的nCount帧\n");
    printf("  /F SaveFile   保持下载Ts文件名\n\n");
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
    LPCSTR lpM3U8File = NULL;
    DWORD lpSkipStart = 0;
    DWORD lpSkipCount = 0;

    for (int i=1; i<argc-1; i++)
    {
        if ( memicmp(argv[i], "/U", 3 ) == 0 ||
            memicmp(argv[i], "-U", 3 ) == 0 )
        {
            lpM3u8Url = argv[++i];
        }
        else if ( memicmp(argv[i], "/R", 3 ) == 0 ||
            memicmp(argv[i], "-R", 3 ) == 0 )
        {
            lpM3U8File = argv[++i];
        }
        else if (memicmp(argv[i], "/F", 3 ) == 0 ||
            memicmp(argv[i], "-F", 3 ) == 0 )
        {
            lpSaveFile = argv[++i];
        }
        else if (memicmp(argv[i], "/Skip", 6) == 0 ||
            memicmp(argv[i], "-Skip", 6) == 0 )
        {
            GetSkip(argv[++i], &lpSkipStart, &lpSkipCount);
        }
    }
    if (lpM3u8Url == NULL && lpM3U8File == NULL)
    {
        usage(argv[0]);
        return -1;
    }
    if (lpSaveFile == NULL)
    {
        lpSaveFile = GetSaveName();
    }
    if (lpM3u8Url == NULL)
    {
        DownM3u8(lpM3U8File, FALSE, lpSaveFile, lpSkipStart, lpSkipCount);
    }
    else
    {
        DownM3u8(lpM3u8Url, TRUE, lpSaveFile, lpSkipStart, lpSkipCount);
    }
#else
//     DownM3u8("http://cdn.luya9.com/ppvod/512DEC0E4B55C857E1FFE628B6CD0401.m3u8", "F:\\av.ts");
    DownM3u8("C:\\Users\\Administrator\\Desktop\\index.m3u8", FALSE, "F:\\三千鸦杀 28.ts");
#endif
    return 0;
}