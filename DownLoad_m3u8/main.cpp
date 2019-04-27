#include <stdio.h>
#include <Shlwapi.h>
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

static void usage(const char* selfname)
{
    const char* lpname = PathFindFileNameA(selfname);
    printf("M3U8下载工具 -- v1.0.0\n\n");
    printf("%s /U M3U8 /F SaveFile  \n", lpname);
    printf("  /U  M3U8      M3U8文件的URL\n");
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
    if (lpM3u8Url == NULL)
    {
        usage(argv[0]);
        return -1;
    }
    if (lpSaveFile == NULL)
    {
        lpSaveFile = GetSaveName();
    }
    DownM3u8(lpM3u8Url, lpSaveFile);
#else
    DownM3u8("http://cdn.luya9.com/ppvod/512DEC0E4B55C857E1FFE628B6CD0401.m3u8", "F:\\av.ts");
#endif
    return 0;
}