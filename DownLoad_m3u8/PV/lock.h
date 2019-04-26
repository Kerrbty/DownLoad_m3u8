#ifndef _LOCK_HEADER_FILE_H_
#define _LOCK_HEADER_FILE_H_
#include <Windows.h>
#include <tchar.h>

class CLock
{
public:
    CLock();

    ~CLock();

    void lock();

    void unlock();
private:
    CRITICAL_SECTION m_cs;

};

#endif // _LOCK_HEADER_FILE_H_