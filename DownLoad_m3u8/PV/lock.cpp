#include "lock.h"

CLock::CLock()
{
    InitializeCriticalSection(&m_cs);
}

CLock::~CLock()
{
    DeleteCriticalSection(&m_cs);
}

void CLock::lock()
{
    EnterCriticalSection(&m_cs);
}

void CLock::unlock()
{
    LeaveCriticalSection(&m_cs);
}
