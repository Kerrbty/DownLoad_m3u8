#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

class CURL
{
public:
    CURL(){};
    CURL(LPCSTR szurl);
    CURL(LPCWSTR szurl);

    BOOL SetUrlA(LPCSTR szurl);
    BOOL SetUrlW(LPCWSTR szurl);

    WORD GetPort();
    LPCWSTR GetServerName();
    LPCWSTR GetObjectName();
    int GetScheme();

    ~CURL();
protected:
    VOID AnalyzeUrl(LPWSTR);
    void ClenData();
private:
    WORD m_port;
    int  m_scheme;
    LPWSTR m_server_name; 
    LPWSTR m_object_name;
};


BOOL WINAPI DecodeURLA(const char* szUrl, char* szDecodeUrl, DWORD dwbuflen); // ½âÂëurl 

BOOL WINAPI EncodeURLA(const char* szUrl, char* szEncodeUrl, DWORD dwbuflen); // ±àÂëurl 



#ifdef __cplusplus
};
#endif
