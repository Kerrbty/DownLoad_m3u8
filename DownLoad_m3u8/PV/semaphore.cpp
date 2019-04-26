#include "semaphore.h"


CSemaphore::CSemaphore(int TotalVal)
{
    m_semaphore = CreateSemaphore(NULL, 0, TotalVal, NULL);
}


CSemaphore::CSemaphore(int TotalVal, int InitVal) // TotalVal ��������InitVal ��ʼ����������
{
    if (InitVal > TotalVal)
    {
        InitVal = TotalVal;
    }
    m_semaphore = CreateSemaphore(NULL, TotalVal-InitVal, TotalVal, NULL);
}

void CSemaphore::P()
{
    WaitForSingleObject(m_semaphore, INFINITE);
}


void CSemaphore::V()
{
    ReleaseSemaphore(m_semaphore, 1, NULL);
}


CSemaphore::~CSemaphore()
{
    CloseHandle(m_semaphore);;
}
