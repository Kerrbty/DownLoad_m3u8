#pragma once

#include "../defs.h"

#define USE_WINHTTP

#ifdef USE_WINHTTP
// xp
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
// xp sp3
#include <WinInet.h>
#pragma comment(lib, "wininet.lib")
#endif


HINTERNET OpenSession(LPCWSTR UserAgent = 0);
HINTERNET Connect(HINTERNET hSession, LPCWSTR szServerAddr, int portNo);
HINTERNET OpenRequest(HINTERNET hConnect, LPCWSTR verb, LPCWSTR objcetName, int scheme);
BOOL AddRequestHeaders(HINTERNET hRequest, LPCWSTR header);
BOOL SendRequest(HINTERNET hRequest, const void* body, DWORD size);
BOOL EndRequest(HINTERNET hRequest);
BOOL QueryInfo(HINTERNET hRequest, DWORD queryId, LPWSTR szBuf, DWORD* cbSize);
BOOL ReadData(HINTERNET hRequest, LPVOID buf, DWORD lenth, DWORD* cbRead);
VOID CloseInternetHandle(HINTERNET hInternet);

// 获取数据到返回buf，必须HeapFree释放内存
LPBYTE GetHttpDataA(LPCSTR szUrl, DWORD* ReturnBufSize);
LPBYTE GetHttpDataW(LPCWSTR szUrl, DWORD* ReturnBufSize);

// Post数据到服务器
LPBYTE PostHttpDataA(LPCSTR szUrl, DWORD* ReturnBufSize, LPBYTE szData, DWORD datalen);
LPBYTE PostHttpDataW(LPCWSTR szUrl, DWORD* ReturnBufSize, LPBYTE szData, DWORD datalen);


#ifdef _UNICODE
#define GetHttpData GetHttpDataW
#define PostHttpData PostHttpDataW
#else
#define GetHttpData GetHttpDataA
#define PostHttpData PostHttpDataA
#endif