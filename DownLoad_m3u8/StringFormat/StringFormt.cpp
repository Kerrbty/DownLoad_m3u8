#include "StringFormt.h"

// UTF8 转 窄字节
PSTR UTF8ToANSI( INOUT PSTR str , IN size_t size)
{
	WCHAR*  pElementText;
	int    iTextLen;
	// wide char to multi char
	iTextLen = MultiByteToWideChar( CP_UTF8,
		0,
		str,
		-1,
		NULL,
		0 );
	
	pElementText = new WCHAR[iTextLen + 1];
	ZeroMemory( pElementText, sizeof(WCHAR)*(iTextLen+1) );

	MultiByteToWideChar( CP_UTF8,
		0,
		str,
		-1,
		pElementText,
		iTextLen );
	
    memset(str, 0, size);
	WideCharToMultiByte( CP_ACP,
		0,
		pElementText,
		-1,
		(PCHAR)str,
		size,
		NULL,
		NULL );

	delete pElementText;
	return str;
}

PSTR ANSIToUTF8( INOUT PSTR str , IN size_t size)
{
    WCHAR*  pElementText;
    int    iTextLen;
    // wide char to multi char
    iTextLen = MultiByteToWideChar( CP_ACP,
        0,
        str,
        -1,
        NULL,
        0 );

    pElementText = new WCHAR[iTextLen + 1];
    ZeroMemory( pElementText, sizeof(WCHAR)*(iTextLen+1) );

    MultiByteToWideChar( CP_ACP,
        0,
        str,
        -1,
        pElementText,
        iTextLen );

    memset(str, 0, size);
    WideCharToMultiByte( CP_UTF8,
        0,
        pElementText,
        -1,
        (PCHAR)str,
        size,
        NULL,
        NULL );

    delete pElementText;
    return str;
}


// 多字节转宽字节，内存需要自行释放
PWSTR MulToWide( LPCSTR str )
{
	PWSTR  pElementText;
	int    iTextLen;

	iTextLen = MultiByteToWideChar( CP_ACP,
		0,
		(PCHAR)str,
		-1,
		NULL,
		0 );
	
	pElementText = 
		(PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (iTextLen+1)*sizeof(WCHAR));

	MultiByteToWideChar( CP_ACP,
		0,
		(PCHAR)str,
		-1,
		pElementText,
		iTextLen );

	return pElementText;
}

PSTR WideToMul( LPCWSTR str )
{
	PSTR  pElementText;
	int    iTextLen;
	
	iTextLen = WideCharToMultiByte( CP_ACP,
		0,
		str,
		-1,
		NULL,
		0,
		NULL,
		NULL);
	
	pElementText = 
		(PSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, iTextLen+1 );
	
	WideCharToMultiByte( CP_ACP,
		0,
		str,
		-1,
		pElementText,
		iTextLen,
		NULL,
		NULL);
	
	return pElementText;
}

//////////////////////////////////////////////////////////////////////////
void CMultiAndWide::SetString(PSTR szStr)
{
    if (szStr == NULL)
    {
        return ;
    }

    if (m_szBuf)
    {
        DelString(m_szBuf);
    }
    if (m_wzBuf)
    {
        DelString(m_wzBuf);
    }

    m_szBuf = 
		(PSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlen(szStr) +1 );
    strcpy(m_szBuf, szStr);
    m_wzBuf = MulToWide(m_szBuf);
}

void CMultiAndWide::SetString(PWSTR wzStr)
{
    if (wzStr == NULL)
    {
        return ;
    }

    if (m_szBuf)
    {
        DelString(m_szBuf);
    }
    if (m_wzBuf)
    {
        DelString(m_wzBuf);
    }
    
    m_wzBuf = 
        (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(wzStr) +1)*sizeof(WCHAR));
    wcscpy(m_wzBuf, wzStr);
    m_szBuf = WideToMul(m_wzBuf);
}


// 查找字符串中给定字符最后出现的位置
LPSTR FindLasteSymbolA(LPSTR CommandLine, CHAR FindWchar)
{
	int Len;
	for ( Len = strlen(CommandLine) ; Len>0; Len-- )
	{
		if (CommandLine[Len] == FindWchar)
		{
			Len++;
			break;
		}
	}
	return &CommandLine[Len];
}

// 查找字符串中给定字符最后出现的位置
LPWSTR FindLasteSymbolW(LPWSTR CommandLine, WCHAR FindWchar)
{
	int Len;
	for ( Len = wcslen(CommandLine) ; Len>0; Len-- )
	{
		if (CommandLine[Len] == FindWchar)
		{
			Len++;
			break;
		}
	}
	return &CommandLine[Len];
}




BOOL IsTextUTF8(const char* str, ULONG length)
{
    DWORD nBytes=0;//UFT8可用1-6个字节编码,ASCII用一个字节
    UCHAR chr;
    BOOL bAllAscii=TRUE; //如果全部都是ASCII, 说明不是UTF-8
    for(int i=0; i<length; ++i)
    {
        chr= *(str+i);
        if( (chr&0x80) != 0 ) // 判断是否ASCII编码,如果不是,说明有可能是UTF-8,ASCII用7位编码,但用一个字节存,最高位标记为0,o0xxxxxxx
            bAllAscii= FALSE;
        if(nBytes==0) //如果不是ASCII码,应该是多字节符,计算字节数
        {
            if(chr>=0x80)
            {
                if(chr>=0xFC&&chr<=0xFD)
                    nBytes=6;
                else if(chr>=0xF8)
                    nBytes=5;
                else if(chr>=0xF0)
                    nBytes=4;
                else if(chr>=0xE0)
                    nBytes=3;
                else if(chr>=0xC0)
                    nBytes=2;
                else
                    return FALSE;

                nBytes--;
            }
        }
        else //多字节符的非首字节,应为 10xxxxxx
        {
            if( (chr&0xC0) != 0x80 )
                return FALSE;

            nBytes--;
        }
    }
    if( nBytes > 0 ) //违返规则
        return FALSE;
    if( bAllAscii ) //如果全部都是ASCII, 说明不是UTF-8
        return FALSE;
    return TRUE;
}