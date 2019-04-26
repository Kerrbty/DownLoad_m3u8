#include "ReadOneLine.h"


CReadLine::CReadLine(PVOID pbuf, DWORD bufsize, BOOL bCopyBuf)
{
    m_pbuf = NULL;
    m_size_buf = 0;
    m_now_offset = 0;
    m_now_point = NULL;
//     m_hFile = INVALID_HANDLE_VALUE;


    if (pbuf != NULL)
    {
        m_size_buf = bufsize;
        if (bCopyBuf)
        {
            m_pbuf = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufsize);
            memcpy(m_pbuf, pbuf, bufsize);
            m_now_point = m_pbuf;
        }
        else
        {
            m_now_point = (PBYTE)pbuf;
        }
    }
}

// 从文件中获取
// CReadLine::CReadLine(LPCSTR filename)
// {
//     m_hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//     if (m_hFile == INVALID_HANDLE_VALUE)
//     {
//         CReadLine(NULL, 0, FALSE);
//     }
//     else
//     {
//         m_FileSize = GetFileSize(m_hFile, NULL);
//         if (m_FileSize != 0)
//         {
//             DWORD dwBytes;
//             if (m_FileSize > 20*1024*1024)
//             {
//                 m_size_buf = 20*1024*1024;
//                 m_pbuf = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_size_buf);
//                 ReadFile(m_pbuf, m_pbuf, m_size_buf, &dwBytes, NULL);
//             }
//             else
//             {
//                 m_size_buf = m_FileSize;
//                 m_pbuf = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_size_buf);
//                 ReadFile(m_hFile, m_pbuf, m_size_buf, &dwBytes, NULL);
//             }
//         }
//         else
//         {
//             m_pbuf = NULL;
//             m_size_buf = 0;
//             m_now_point = NULL;
//         }
//     }
// }



// DWORD m_size_buf;  // m_buf 申请的内存长度
// PBYTE m_pbuf;      // 申请的内存指针
// PBYTE m_now_point; // 当前读取位置
// 
// HANDLE m_hFile;    // 文件句柄
// DWORD m_FileSize;  // 文件的真实长度
char* CReadLine::getLine(LPSTR pbuf, PDWORD ullen)
{
    if (m_now_point == NULL || ullen == NULL)
    {
        return NULL;
    }

    if (pbuf == NULL)
    {
        *ullen = 0;
    }
    
    DWORD dwlen = 0;
    memset(pbuf, 0, *ullen);
    LPBYTE p = (LPBYTE)pbuf;
    DWORD read_buf_size = *ullen;
//     if( m_hFile == INVALID_HANDLE_VALUE)
//     {
//         // 从文件中读取
//     }
//     else
    {
        // 内存读取
        while(m_now_offset < m_size_buf)
        {
            if (*m_now_point == 0x0D)
            {
                m_now_point++;
                m_now_offset++;
                if (*m_now_point == 0x0A)
                {
                    m_now_point++;
                    m_now_offset++;
                }
                break;
            }
            if (*m_now_point == 0x0A)
            {
                m_now_point++;
                m_now_offset++;
                break;
            }
            else if (*m_now_point == 0)
            {
                break;
            }
            else
            {
                dwlen++;
                if (read_buf_size > dwlen)
                {
                    *p++ = *m_now_point;
                }
            }
            m_now_point++;
            m_now_offset++;
        }
        *ullen = dwlen;
    }

    if (dwlen == 0)
    {
        return NULL;
    }

    if (read_buf_size > dwlen)
    {
        return pbuf;
    }
    return NULL;
}




CReadLine::~CReadLine()
{
    if (m_pbuf)
    {
        HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, m_pbuf);
    }

//     if (m_hFile != INVALID_HANDLE_VALUE)
//     {
//         CloseHandle(m_hFile);
//     }
}
