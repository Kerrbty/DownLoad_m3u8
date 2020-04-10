// CommonDef.h ͨ�õ�windows�궨�� 
// 
#ifndef _ZXLY_COMMAND_DEFINES_HEADER_H_
#define _ZXLY_COMMAND_DEFINES_HEADER_H_
#ifdef __cplusplus
#define EXTERN_C   extern "C"
#else  // __cplusplus 
#define EXTERN_C
#endif // __cplusplus 

#include <windows.h>
#include <tchar.h>

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef INOUT
#define INOUT
#endif

#define AllocMemory(_nsize)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _nsize)  // ���ֽ������ڴ棬��C���п�δ��ʼ������¾Ϳ��� 
#define ReAllocMemory(_addr, _nsize)  HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _addr, _nsize) // ��ԭ���������ӡ����ٷ��ڴ� 
#define FreeMemory(_addr)   { if (_addr) { HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, _addr); _addr=NULL; } }  // �ͷ��ڴ�
#define DeleteHandle(_handle) {if(_handle != INVALID_HANDLE_VALUE) { CloseHandle(_handle); _handle = INVALID_HANDLE_VALUE;}} // �رվ�� 

// ���ļ� 
#define FileOpenA(_name, _openf, _sharef, _createf)  CreateFileA(_name, _openf, _sharef, NULL, _createf, FILE_ATTRIBUTE_NORMAL, NULL)  
#define FileOpenW(_name, _openf, _sharef, _createf)  CreateFileW(_name, _openf, _sharef, NULL, _createf, FILE_ATTRIBUTE_NORMAL, NULL)

#ifdef _UNICODE
#define FileOpen FileOpenW 
#else /* _UNICODE */
#define FileOpen FileOpenA 
#endif /* _UNICODE */


#if _MSC_VER < 1400 
EXTERN_C BOOL WINAPI GetFileSizeEx(HANDLE, PLARGE_INTEGER);
EXTERN_C BOOL WINAPI SetFilePointerEx(HANDLE, LARGE_INTEGER, PLARGE_INTEGER,DWORD);
#endif /* _MSC_VER */ 


#endif /* _ZXLY_COMMAND_DEFINES_HEADER_H_ */ 