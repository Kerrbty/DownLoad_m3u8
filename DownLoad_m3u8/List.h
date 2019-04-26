#ifndef _H_HH_LIST_HH_H_
#define _H_HH_LIST_HH_H_
#include <windows.h>

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = PtrToUlong((ListHead)))


#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif

FORCEINLINE 
VOID
_InitializeListHead(
    IN PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

BOOLEAN
FORCEINLINE
_IsListEmpty(
    IN const LIST_ENTRY * ListHead
    )
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

FORCEINLINE
BOOLEAN
_RemoveEntryList(
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;
    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

// 返回当前节点的前一个(Flink)节点 
FORCEINLINE
PLIST_ENTRY
_RemoveHeadList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;
    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}
 
// 删除返回当前列表节点的后一个(Blink)节点 
FORCEINLINE
PLIST_ENTRY
_RemoveTailList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;
    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

// 该函数作用是在 ListHead 列表之前插入节点 Entry 
FORCEINLINE
VOID
_InsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}


// 该函数作用是在 ListHead 列表后面加一个节点 Entry 
FORCEINLINE
VOID
_InsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;
    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE
VOID
_AppendTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListToAppend
    )
{
    PLIST_ENTRY ListEnd = ListHead->Blink;
    ListHead->Blink->Flink = ListToAppend;
    ListHead->Blink = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink = ListEnd;
}

FORCEINLINE
PSINGLE_LIST_ENTRY
_PopEntryList(
    PSINGLE_LIST_ENTRY ListHead
    )
{
    PSINGLE_LIST_ENTRY FirstEntry;
    FirstEntry = ListHead->Next;
    if (FirstEntry != NULL) {
        ListHead->Next = FirstEntry->Next;
    }
    return FirstEntry;
}

FORCEINLINE
VOID
_PushEntryList(
    PSINGLE_LIST_ENTRY ListHead,
    PSINGLE_LIST_ENTRY Entry
    )
{
    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;
}

FORCEINLINE
BOOLEAN
_RemoveOneList(
	IN PLIST_ENTRY  ListHead
    )
{
	if (ListHead->Flink == NULL && _IsListEmpty(ListHead))
	{
		return FALSE;
	}
	PLIST_ENTRY MiddleEntry = ListHead->Flink;
	MiddleEntry->Blink = ListHead->Blink;
	MiddleEntry = ListHead->Blink;
	MiddleEntry->Flink = ListHead->Flink;
	return TRUE;
}

#endif // _H_HH_LIST_HH_H_