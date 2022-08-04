#pragma once
#ifndef _MEM_H
#define _MEM_H
#include "def.h"
NTSTATUS NTAPI MmCopyVirtualMemory
(
	PEPROCESS SourceProcess,
	PVOID SourceAddress,
	PEPROCESS TargetProcess,
	PVOID TargetAddress,
	SIZE_T BufferSize,
	KPROCESSOR_MODE PreviousMode,
	PSIZE_T ReturnSize
);

PVOID get_system_module_base(LPCSTR moudle_name);
PVOID get_system_module_export(LPCSTR moudle_name, LPCSTR routine_name);
BOOL ke_write_memory(PVOID address, PVOID buffer, size_t size);
BOOL ke_write_to_read_memory(PVOID address, PVOID buffer, size_t size);
PVOID GetProcessBaseAddress(int pid);

//NTSTATUS ke_read_process_memory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size);

NTSTATUS WritePhysicalAddress(PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten);
UINT64 TranslateLinearAddress(UINT64 directoryTableBase, UINT64 virtualAddress);
NTSTATUS ReadPhysicalAddress(PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead);
NTSTATUS ReadProcessMemory(int pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read);
NTSTATUS WriteProcessMemory(int pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written);
#endif
