#ifndef _READ_ONE_LINE_HEADER_HH_
#define _READ_ONE_LINE_HEADER_HH_
#include <windows.h>

class CReadLine
{
public:
    CReadLine(PVOID pbuf, DWORD bufsize, BOOL bCopyBuf);
//     CReadLine(LPCSTR filename);

    LPSTR getLine(LPSTR pbuf, PDWORD ullen);

    ~CReadLine();
protected:
    ;
private:
    DWORD m_size_buf;  // m_buf 申请的内存长度
    PBYTE m_pbuf;      // 申请的内存指针
    PBYTE m_now_point; // 当前读取位置
    DWORD m_now_offset;// 读取位于文件首的便宜

//     // 文件标志
//     HANDLE m_hFile;    // 文件句柄
//     DWORD m_FileSize;  // 文件的真实长度
};



#endif // _READ_ONE_LINE_HEADER_HH_