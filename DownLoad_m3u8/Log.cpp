#include "Log.h"
#include "stdio.h"
#include "pv/lock.h"

// 日志文件
CLock LogFileLock;

static bool create_directoryW(const WCHAR* szpath)
{
    if (szpath == NULL)
    {
        return false;
    }

    size_t len = wcslen(szpath);
    LPWSTR szdir = (LPWSTR)AllocMemory((len+1)*sizeof(WCHAR));
    if (szdir)
    {
        LPWSTR p = szdir;
        LPCWSTR q = szpath;

        memset(szdir, 0, (len+1)*sizeof(WCHAR));
        while(*q!= NULL)
        {
            if (*q == L'\\' || *q == L'/')
            {
                CreateDirectoryW(szdir, NULL);
            }
            *p++ = *q++;
        }
        CreateDirectoryW(szdir, NULL);
        ASSERT_HEAP(szdir);
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

        LPWSTR filename = (LPWSTR)AllocMemory(1024*sizeof(WCHAR));
        GetModuleFileNameW(NULL, filename, 1024);
        size_t szlen = wcslen(filename);
        if (szlen > 0)
        {
            szlen--;
            while(filename[szlen] != L'\\')
            {
                szlen--;
            }
            wcscpy(filename+szlen, L"\\Log\\");
        }
        create_directoryW(filename); 
        szlen = wcslen(filename);
        wsprintfW(filename+szlen, L"LogInfo-%04d%02d%02d-%02d%02d.log", SystemTime.wYear,  SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute);
        ASSERT_HEAP(filename);
        LogFileLock.lock();
        if (file == NULL)
        {
            file = _wfopen(filename, L"w+");
        }
        LogFileLock.unlock();
        FreeMemory(filename);
    }
    if (file != NULL)
    { 
        LogFileLock.lock();
        fprintf(file, "[%04u/%02u/%02u %02u:%02u:%02u] [threadid: %u] ", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
            SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, GetCurrentThreadId());

        rvl = vfprintf(file, Format, va);
        fflush(file);
        LogFileLock.unlock();
    }
    va_end(va);
    return rvl;
}