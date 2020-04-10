#ifndef _FORMAT_LANGUAGE_STRING_H_
#define _FORMAT_LANGUAGE_STRING_H_
#include "../defs.h"

#define DelString(_a)  FreeMemory(_a)


// 多字节转宽字节，内存需要自行释放
PWSTR MulToWide( IN LPCSTR str );
PSTR WideToMul( IN LPCWSTR str );


class CMultiAndWide
{
public:
    CMultiAndWide()
    {
        m_szBuf = NULL;
        m_wzBuf = NULL;
    }

    CMultiAndWide(PSTR szStr)
    {
        SetString(szStr);
    }

    CMultiAndWide(PWSTR wzStr)
    {
        SetString(wzStr);
    }

    void SetString(PSTR szStr);
    void SetString(PWSTR wzStr);

    PSTR GetMultiByte()
    {
        return m_szBuf;
    }

    PWSTR GetWideChar()
    {
        return m_wzBuf;
    }

    ~CMultiAndWide()
    {
        if (m_szBuf)
        {
            DelString(m_szBuf);
        }
        if (m_wzBuf)
        {
            DelString(m_wzBuf);
        }
    }

private:
    PSTR m_szBuf;
    PWSTR m_wzBuf;
};


// UTF8 转 窄字节
PSTR UTF8ToANSI( INOUT PSTR str , IN size_t size);
PSTR ANSIToUTF8( INOUT PSTR str , IN size_t size);

LPSTR FindLasteSymbolA(LPSTR CommandLine, CHAR FindWchar); // 查找字符串中给定字符最后出现的位置
LPWSTR FindLasteSymbolW(LPWSTR CommandLine, WCHAR FindWchar); // 查找字符串中给定字符最后出现的位置


BOOL IsTextUTF8(const char* str, ULONG length);  // 判断文本是否为utf-8编码 


#ifdef _UNICODE
#define FindLasteSymbol FindLasteSymbolW
#else
#define FindLasteSymbol FindLasteSymbolA
#endif


#endif  // _FORMAT_LANGUAGE_STRING_H_