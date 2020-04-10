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
    DWORD m_size_buf;  // m_buf ������ڴ泤��
    PBYTE m_pbuf;      // ������ڴ�ָ��
    PBYTE m_now_point; // ��ǰ��ȡλ��
    DWORD m_now_offset;// ��ȡλ���ļ��׵ı���

//     // �ļ���־
//     HANDLE m_hFile;    // �ļ����
//     DWORD m_FileSize;  // �ļ�����ʵ����
};



#endif // _READ_ONE_LINE_HEADER_HH_