#include "HttpPost.h"
#include "AnalyzeURL.h"
#include "../StringFormat/StringFormt.h"
#include <stdlib.h>
#ifdef _DEBUG
#include <stdio.h>
#endif

HINTERNET OpenSession(LPCWSTR UserAgent)
{
#ifdef USE_WINHTTP
    return WinHttpOpen( UserAgent,
                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                        WINHTTP_NO_PROXY_NAME,
                        WINHTTP_NO_PROXY_BYPASS,
                        0);
#else
    return InternetOpenW(UserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
#endif
}


HINTERNET Connect(HINTERNET hSession, LPCWSTR szServerAddr, int portNo)
{
#ifdef USE_WINHTTP
    return WinHttpConnect(hSession, szServerAddr, (INTERNET_PORT)portNo, 0);
#else
    return InternetConnectW(hSession, szServerAddr, (INTERNET_PORT)portNo, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
#endif
}


HINTERNET OpenRequest(HINTERNET hConnect, LPCWSTR verb, LPCWSTR objcetName, int scheme)
{
    DWORD flags = 0;
#ifdef USE_WINHTTP
    if (scheme == INTERNET_SCHEME_HTTPS)
    {
        flags |= WINHTTP_FLAG_SECURE;
    }
    return WinHttpOpenRequest(hConnect, verb, objcetName, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
#else
    if (scheme == INTERNET_SCHEME_HTTPS)
    {
        flags |= INTERNET_FLAG_SECURE;
    }
    return HttpOpenRequestW(hConnect, verb, objcetName, NULL, NULL, NULL, flags, 0);
#endif
}


BOOL AddRequestHeaders(HINTERNET hRequest, LPCWSTR header)
{
    DWORD len = wcslen(header);
#ifdef USE_WINHTTP
    return WinHttpAddRequestHeaders(hRequest, header, len, WINHTTP_ADDREQ_FLAG_ADD);
#else
    return HttpAddRequestHeadersW(hRequest, header, len, HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE);
#endif
}


BOOL SendRequest(HINTERNET hRequest, const void* body, DWORD size)
{
#ifdef USE_WINHTTP
    return WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)body, size, size, 0);
#else
    return HttpSendRequest(hRequest, NULL, NULL, (LPVOID)body, size);
#endif
}


BOOL EndRequest(HINTERNET hRequest)
{
#ifdef USE_WINHTTP
    return WinHttpReceiveResponse(hRequest, 0);
#else
    return TRUE;
#endif
}


BOOL QueryInfo(HINTERNET hRequest, DWORD queryId, LPWSTR szBuf, DWORD* cbSize)
{
#ifdef USE_WINHTTP
    return WinHttpQueryHeaders(hRequest, queryId, 0, szBuf, cbSize, 0);
#else
    return HttpQueryInfoW(hRequest, queryId, szBuf, cbSize, 0);
#endif
}

BOOL ReadData(HINTERNET hRequest, LPVOID buf, DWORD lenth, DWORD* cbRead)
{
#ifdef USE_WINHTTP
    if( WinHttpReadData(hRequest, buf, lenth, cbRead) )
    {
        if ( IsTextUTF8((LPSTR)buf, *cbRead) )
        {
            UTF8ToANSI((LPSTR)buf, *cbRead);
        }
        return TRUE;
    }

#else
    if( InternetReadFile(hRequest, buf, lenth, cbRead) )
    {
        if ( IsTextUTF8((LPSTR)buf, *cbRead) )
        {
            UTF8ToANSI((LPSTR)buf, *cbRead);
        }
        return TRUE;
    }
#endif
    return FALSE;
}


VOID CloseInternetHandle(HINTERNET hInternet)
{
    if (hInternet)
    {
#ifdef USE_WINHTTP
        WinHttpCloseHandle(hInternet);
#else
        InternetCloseHandle(hInternet);
#endif
    }
}


LPBYTE GetHttpDataW(LPCWSTR szUrl, DWORD* ReturnBufSize)
{
    BOOL iSuccess = FALSE;
    LPBYTE pData = NULL;
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

#ifdef USE_WINHTTP
    int contextLengthId = WINHTTP_QUERY_CONTENT_LENGTH;
    int statusCodeId = WINHTTP_QUERY_STATUS_CODE;
    int statusTextId = WINHTTP_QUERY_STATUS_TEXT;
#else
    int contextLengthId = HTTP_QUERY_CONTENT_LENGTH;
    int statusCodeId = HTTP_QUERY_STATUS_CODE;
    int statusTextId = HTTP_QUERY_STATUS_TEXT;
#endif

    const DWORD dwHeaderSize = 2048;
    PWSTR szHeader = (PWSTR)AllocMemory(dwHeaderSize*sizeof(WCHAR));
    DWORD cbSize = 0;

    CURL connect_url(szUrl);
    do
    {
        // Open session.
        hSession = OpenSession(L"Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.102 Safari/537.36"); //  HttpPost by Odin Ads
        if (hSession == NULL)
        {
#ifdef _DEBUG
            printf("hSession Error\n");
#endif
            break;
        }

        // test-tool.xianshuabao.com
        hConnect = Connect(hSession, connect_url.GetServerName(), connect_url.GetPort());
        if (hConnect == NULL)
        {
#ifdef _DEBUG
            printf("Connect Error\n");
#endif
            break;;
        }

        // Open request.
        // api/ad/getimages
        hRequest = OpenRequest(hConnect, L"GET", connect_url.GetObjectName(), connect_url.GetScheme() );
        if (hRequest == NULL)
        {
#ifdef _DEBUG
            printf("OpenRequest Error\n");
#endif
            break;
        }

//         AddRequestHeaders(hRequest, L"Accept: */*\r\n");
//         AddRequestHeaders(hRequest, L"Accept-Encoding: gzip, deflate\r\n");
//         AddRequestHeaders(hRequest, L"Connection: Keep-Alive\r\n");
//         AddRequestHeaders(hRequest, L"DNT: 1\r\n");
        // Send post data.
        if (!SendRequest(hRequest, NULL, 0))
        {
#ifdef _DEBUG
            printf("SendRequest Error: %d\n", GetLastError());
#endif
            break;
        }

        // End request
        if (!EndRequest(hRequest))
        {
#ifdef _DEBUG
            printf("EndRequest Error: %d\n", GetLastError());
#endif
            break;
        }

        // Query header info.
        cbSize = dwHeaderSize*sizeof(WCHAR);
        memset(szHeader, 0, cbSize);
        if (QueryInfo(hRequest, statusCodeId, szHeader, &cbSize))
        {
#ifdef _DEBUG
            printf("statusCodeId:%S\n", szHeader);
#endif
            if (wcscmp(szHeader, L"200") != 0)
            {
                break;
            }
        }

        cbSize = dwHeaderSize*sizeof(WCHAR);
        memset(szHeader, 0, cbSize);
        if (QueryInfo(hRequest, statusTextId, szHeader, &cbSize))
        {
#ifdef _DEBUG
            printf("statusTextId:%S\n", szHeader);
#endif
            if (wcsicmp(szHeader, L"OK") != 0)
            {
                break;
            }
        }

        cbSize = dwHeaderSize*sizeof(WCHAR);
        memset(szHeader, 0, cbSize);
        if (QueryInfo(hRequest, contextLengthId, szHeader, &cbSize))
        {
#ifdef _DEBUG
            printf("Content length:[%S]\n", szHeader);
#endif
        }

        DWORD ilenth = _wtol(szHeader);
        if (ilenth == 0)
        {
            ilenth = 40*1024*1024;    // 40MB
        }
        if (ReturnBufSize != NULL)
        {
            *ReturnBufSize = ilenth;
        }
        pData = (LPBYTE)AllocMemory(ilenth+1);

        // read data.
        if ( pData != NULL)
        {

            if (ReadData(hRequest, pData, ilenth, &cbSize) == TRUE && cbSize > 0 )
            {
                iSuccess = TRUE;
#ifdef _DEBUG
//                     printf("%s\n", pData);
#endif
                if (cbSize < ilenth)
                {
                    ReAllocMemory(pData, cbSize+1);
                    if (ReturnBufSize != NULL)
                    {
                        *ReturnBufSize = cbSize;
                    }
                }
            }
        }
    } while (FALSE);

    // 没成功，释放内存
    if (!iSuccess)
    {
        FreeMemory(pData);
    }
    FreeMemory(szHeader);
    CloseInternetHandle(hRequest);
    CloseInternetHandle(hConnect);
    CloseInternetHandle(hSession);
    return pData;
}

LPBYTE GetHttpDataA(LPCSTR szUrl, DWORD* ReturnBufSize)
{
    PWSTR wszUrl = MulToWide(szUrl);
    LPBYTE lpRetbuf = GetHttpDataW(wszUrl, ReturnBufSize);
    DelString(wszUrl);
    return lpRetbuf;
}

LPBYTE PostHttpDataW(LPCWSTR szUrl, DWORD* ReturnBufSize, LPBYTE szData, DWORD datalen)
{
    BOOL iSuccess = FALSE;
    LPBYTE pData = NULL;
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

#ifdef USE_WINHTTP
    int contextLengthId = WINHTTP_QUERY_CONTENT_LENGTH;
    int statusCodeId = WINHTTP_QUERY_STATUS_CODE;
    int statusTextId = WINHTTP_QUERY_STATUS_TEXT;
#else
    int contextLengthId = HTTP_QUERY_CONTENT_LENGTH;
    int statusCodeId = HTTP_QUERY_STATUS_CODE;
    int statusTextId = HTTP_QUERY_STATUS_TEXT;
#endif

    const DWORD dwHeaderSize = 2048;
    PWSTR szHeader = (PWSTR)AllocMemory(dwHeaderSize*sizeof(WCHAR));
    DWORD cbSize = 0;

    CURL connect_url(szUrl);

    do
    {
        // Open session.
        hSession = OpenSession(L"Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.102 Safari/537.36");
        if (hSession == NULL)
        {
            break;
        }

        // test-tool.xianshuabao.com
        hConnect = Connect(hSession, connect_url.GetServerName(), connect_url.GetPort());
        if (hConnect == NULL)
        {
            break;;
        }

        // Open request.
        // api/ad/getimages
        hRequest = OpenRequest(hConnect, L"Post", connect_url.GetObjectName(), connect_url.GetScheme() );
        if (hRequest == NULL)
        {
            break;
        }

        // application/raw
        if (!AddRequestHeaders(hRequest, L"Content-type: application/x-www-form-urlencoded; Charset=UTF-8\r\n"))
        {
            break;
        }

        // application/raw
        if (!AddRequestHeaders(hRequest, L"Accept: */*\r\n"))
        {
            break;
        }

        // Send post data.
        if (!SendRequest(hRequest, szData, datalen))
        {
            break;
        }

        // End request
        if (!EndRequest(hRequest))
        {
            break;
        }

        // Query header info.
        cbSize = dwHeaderSize*sizeof(WCHAR);
        memset(szHeader, 0, cbSize);
        if (QueryInfo(hRequest, statusCodeId, szHeader, &cbSize))
        {
            if (wcscmp(szHeader, L"200") != 0)
            {
                break;
            }
        }

        cbSize = dwHeaderSize*sizeof(WCHAR);
        memset(szHeader, 0, cbSize);
        if (QueryInfo(hRequest, statusTextId, szHeader, &cbSize))
        {
            if (wcsicmp(szHeader, L"OK") != 0)
            {
                break;
            }
        }

        cbSize = dwHeaderSize*sizeof(WCHAR);
        memset(szHeader, 0, cbSize);
        if (QueryInfo(hRequest, contextLengthId, szHeader, &cbSize))
        {
#ifdef _DEBUG
            printf("Content length:[%S]\n", szHeader);
#endif
        }

        DWORD ilenth = _wtol(szHeader);
        if (ilenth == 0)
        {
            ilenth = 40*1024*1024;  // 40MB
        }
        if (ReturnBufSize != NULL)
        {
            *ReturnBufSize = ilenth;
        }
        pData = (LPBYTE)AllocMemory(ilenth+1);

        // read data.
        if ( pData != NULL)
        {

            if (ReadData(hRequest, pData, ilenth, &cbSize) == TRUE && cbSize > 0 )
            {
                iSuccess = TRUE;
#ifdef _DEBUG
//                     printf("%s\n", pData);
#endif
                if (cbSize < ilenth)
                {
                    ReAllocMemory(pData, cbSize+1);
                    if (ReturnBufSize != NULL)
                    {
                        *ReturnBufSize = cbSize;
                    }
                }
            }
        }
    } while (FALSE);


    // 没成功，释放内存
    if (!iSuccess)
    {
        FreeMemory(pData);
    }
    FreeMemory(szHeader);
    CloseInternetHandle(hRequest);
    CloseInternetHandle(hConnect);
    CloseInternetHandle(hSession);
    return pData;
}


LPBYTE PostHttpDataA(LPCSTR szUrl, DWORD* ReturnBufSize, LPBYTE szData, DWORD datalen)
{
    PWSTR wszUrl = MulToWide(szUrl);
    LPBYTE lpRetbuf = PostHttpDataW(wszUrl, ReturnBufSize, szData, datalen);
    DelString(wszUrl);
    return lpRetbuf;
}