#pragma once
#ifndef _KEHOK
#define _KEHOK
#include "mem.h"

BOOL ke_call_function(PVOID fn_address);
NTSTATUS hook_handle(PVOID, PVOID data, PVOID, PVOID);

typedef struct RTL_STRUCT {
    SIZE_T* read;
    UINT_PTR address;
    SIZE_T size;
    ULONG pid;

    enum IOMode {
        IOMODE_Write,
        IOMODE_Read,
        IOMODE_ReqBase,
        IOMODE_Unhook
    } io_mode;

    PVOID out;
    PCWSTR mod_name;
    UINT_PTR base_address;
} RtlStruct;

#endif
