#include "StringFormt.h"

// UTF8 ת խ�ֽ�
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


// ���ֽ�ת���ֽڣ��ڴ���Ҫ�����ͷ�
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


// �����ַ����и����ַ������ֵ�λ��
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

// �����ַ����и����ַ������ֵ�λ��
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
    DWORD nBytes=0;//UFT8����1-6���ֽڱ���,ASCII��һ���ֽ�
    UCHAR chr;
    BOOL bAllAscii=TRUE; //���ȫ������ASCII, ˵������UTF-8
    for(int i=0; i<length; ++i)
    {
        chr= *(str+i);
        if( (chr&0x80) != 0 ) // �ж��Ƿ�ASCII����,�������,˵���п�����UTF-8,ASCII��7λ����,����һ���ֽڴ�,���λ���Ϊ0,o0xxxxxxx
            bAllAscii= FALSE;
        if(nBytes==0) //�������ASCII��,Ӧ���Ƕ��ֽڷ�,�����ֽ���
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
        else //���ֽڷ��ķ����ֽ�,ӦΪ 10xxxxxx
        {
            if( (chr&0xC0) != 0x80 )
                return FALSE;

            nBytes--;
        }
    }
    if( nBytes > 0 ) //Υ������
        return FALSE;
    if( bAllAscii ) //���ȫ������ASCII, ˵������UTF-8
        return FALSE;
    return TRUE;
}