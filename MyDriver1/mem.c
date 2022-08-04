#include "mem.h"

PVOID get_system_module_base(LPCSTR moudle_name) {
    ULONG byte = 0;
    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, byte, &byte);
    if (!byte)
        return NULL;
    RTL_PROCESS_MODULES* modules = (RTL_PROCESS_MODULES*)ExAllocatePoolWithTag(NonPagedPool, byte, 0x4E554C4C);
    status = ZwQuerySystemInformation(SystemModuleInformation, modules, byte, &byte);

    RTL_PROCESS_MODULE_INFORMATION* module = modules->Modules;
    PVOID mod_base = 0, mod_size = 0;
    for (ULONG i = 0; i < modules->NumberOfModules; i++) {
        if (strcmp((CHAR*)module[i].FullPathName, moudle_name) == 0) {
            mod_base = module[i].ImageBase;
            mod_size = (PVOID)module[i].ImageSize;
            break;
        }
    }
    if (modules != NULL)
        ExFreePoolWithTag(modules, NULL);
    if (mod_base <= NULL)
        return NULL;
    return mod_base;
}

PVOID get_system_module_export(LPCSTR moudle_name, LPCSTR routine_name) {
    PVOID lp_module = get_system_module_base(moudle_name);
    if (lp_module == NULL)
        return NULL;
    return RtlFindExportedRoutineByName(lp_module, routine_name);
}

BOOL ke_write_memory(PVOID address, PVOID buffer, size_t size) {
    if (!RtlCopyMemory(address, buffer, size)) {
        return FALSE;
    }
    return TRUE;
}

BOOL ke_write_to_read_memory(PVOID address, PVOID buffer, size_t size) {
    PMDL mdl = IoAllocateMdl(address, size, FALSE, FALSE, NULL);
    if (!mdl)
        return FALSE;
    MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
    PVOID mapping = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
    MmProtectMdlSystemAddress(mdl, PAGE_READWRITE);
    ke_write_memory(mapping, buffer, size);
    
    MmUnmapLockedPages(mapping, mdl);
    MmUnlockPages(mdl);
    IoFreeMdl(mdl);
    return TRUE;
}

NTKERNELAPI
PVOID
PsGetProcessSectionBaseAddress(
    __in PEPROCESS Process
);

PVOID GetProcessBaseAddress(int pid) {
    PEPROCESS pProcess = NULL;
    if (pid == 0) return STATUS_UNSUCCESSFUL;

    NTSTATUS NtRet = PsLookupProcessByProcessId(pid, &pProcess);
    if (NtRet != STATUS_SUCCESS) return NtRet;

    PVOID Base = PsGetProcessSectionBaseAddress(pProcess);
    ObDereferenceObject(pProcess);
    return Base;
}

//https://ntdiff.github.io/
#define WINDOWS_1803 17134
#define WINDOWS_1809 17763
#define WINDOWS_1903 18362
#define WINDOWS_1909 18363
#define WINDOWS_2004 19041
#define WINDOWS_20H2 19569
#define WINDOWS_21H1 20180

DWORD GetUserDirectoryTableBaseOffset() {
    RTL_OSVERSIONINFOW ver = {0};
    RtlGetVersion(&ver);

    switch (ver.dwBuildNumber) {
    case WINDOWS_1803:
        return 0x0278;
        break;
    case WINDOWS_1809:
        return 0x0278;
        break;
    case WINDOWS_1903:
        return 0x0280;
        break;
    case WINDOWS_1909:
        return 0x0280;
        break;
    case WINDOWS_2004:
        return 0x0388;
        break;
    case WINDOWS_20H2:
        return 0x0388;
        break;
    case WINDOWS_21H1:
        return 0x0388;
        break;
    default:
        return 0x0388;
    }
}

//check normal dirbase if 0 then get from UserDirectoryTableBas
ULONG_PTR GetProcessCr3(PEPROCESS pProcess) {
    PUCHAR process = (PUCHAR)pProcess;
    ULONG_PTR process_dirbase = *(PULONG_PTR)(process + 0x28); //dirbase x64, 32bit is 0x18
    if (process_dirbase == 0) {
        DWORD UserDirOffset = GetUserDirectoryTableBaseOffset();
        ULONG_PTR process_userdirbase = *(PULONG_PTR)(process + UserDirOffset);
        return process_userdirbase;
    }
    return process_dirbase;
}

ULONG_PTR GetKernelDirBase() {
    PUCHAR process = (PUCHAR)PsGetCurrentProcess();
    ULONG_PTR cr3 = *(PULONG_PTR)(process + 0x28); //dirbase x64, 32bit is 0x18
    return cr3;
}

NTSTATUS ReadVirtual(UINT64 dirbase, UINT64 address, UINT8* buffer, SIZE_T size, SIZE_T* read) {
    UINT64 paddress = TranslateLinearAddress(dirbase, address);
    return ReadPhysicalAddress(paddress, buffer, size, read);
}

NTSTATUS WriteVirtual(UINT64 dirbase, UINT64 address, UINT8* buffer, SIZE_T size, SIZE_T* written) {
    UINT64 paddress = TranslateLinearAddress(dirbase, address);
    return WritePhysicalAddress(paddress, buffer, size, written);
}

NTSTATUS ReadPhysicalAddress(PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead) {
    MM_COPY_ADDRESS AddrToRead = {0};
    AddrToRead.PhysicalAddress.QuadPart = TargetAddress;
    return MmCopyMemory(lpBuffer, AddrToRead, Size, MM_COPY_MEMORY_PHYSICAL, BytesRead);
}

//MmMapIoSpaceEx limit is page 4096 byte
NTSTATUS WritePhysicalAddress(PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten) {
    if (!TargetAddress)
        return STATUS_UNSUCCESSFUL;

    PHYSICAL_ADDRESS AddrToWrite = {0};
    AddrToWrite.QuadPart = TargetAddress;

    PVOID pmapped_mem = MmMapIoSpaceEx(AddrToWrite, Size, PAGE_READWRITE);

    if (!pmapped_mem)
        return STATUS_UNSUCCESSFUL;

    memcpy(pmapped_mem, lpBuffer, Size);

    *BytesWritten = Size;
    MmUnmapIoSpace(pmapped_mem, Size);
    return STATUS_SUCCESS;
}

#define PAGE_OFFSET_SIZE 12
static const UINT64 PMASK = (~0xfull << 8) & 0xfffffffffull;

UINT64 TranslateLinearAddress(UINT64 directoryTableBase, UINT64 virtualAddress) {
    directoryTableBase &= ~0xf;

    UINT64 pageOffset = virtualAddress & ~(~0ul << PAGE_OFFSET_SIZE);
    UINT64 pte = ((virtualAddress >> 12) & (0x1ffll));
    UINT64 pt = ((virtualAddress >> 21) & (0x1ffll));
    UINT64 pd = ((virtualAddress >> 30) & (0x1ffll));
    UINT64 pdp = ((virtualAddress >> 39) & (0x1ffll));

    SIZE_T readsize = 0;
    UINT64 pdpe = 0;
    ReadPhysicalAddress(directoryTableBase + 8 * pdp, &pdpe, sizeof(pdpe), &readsize);
    if (~pdpe & 1)
        return 0;

    UINT64 pde = 0;
    ReadPhysicalAddress((pdpe & PMASK) + 8 * pd, &pde, sizeof(pde), &readsize);
    if (~pde & 1)
        return 0;

    /* 1GB large page, use pde's 12-34 bits */
    if (pde & 0x80)
        return (pde & (~0ull << 42 >> 12)) + (virtualAddress & ~(~0ull << 30));

    UINT64 pteAddr = 0;
    ReadPhysicalAddress((pde & PMASK) + 8 * pt, &pteAddr, sizeof(pteAddr), &readsize);
    if (~pteAddr & 1)
        return 0;

    /* 2MB large page */
    if (pteAddr & 0x80)
        return (pteAddr & PMASK) + (virtualAddress & ~(~0ull << 21));

    virtualAddress = 0;
    ReadPhysicalAddress((pteAddr & PMASK) + 8 * pte, &virtualAddress, sizeof(virtualAddress), &readsize);
    virtualAddress &= PMASK;

    if (!virtualAddress)
        return 0;

    return virtualAddress + pageOffset;
}

NTSTATUS ReadProcessMemory(int pid, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read) {
    PEPROCESS pProcess = NULL;
    if (pid == 0) return STATUS_UNSUCCESSFUL;

    NTSTATUS NtRet = PsLookupProcessByProcessId(pid, &pProcess);
    if (NtRet != STATUS_SUCCESS) return NtRet;

    ULONG_PTR process_dirbase = GetProcessCr3(pProcess);
    ObDereferenceObject(pProcess);

    SIZE_T CurOffset = 0;
    SIZE_T TotalSize = size;
    while (TotalSize) {
        UINT64 CurPhysAddr = TranslateLinearAddress(process_dirbase, (ULONG64)Address + CurOffset);
        if (!CurPhysAddr) return STATUS_UNSUCCESSFUL;

        ULONG64 ReadSize = min(PAGE_SIZE - (CurPhysAddr & 0xFFF), TotalSize);
        SIZE_T BytesRead = 0;
        NtRet = ReadPhysicalAddress(CurPhysAddr, (PVOID)((ULONG64)AllocatedBuffer + CurOffset), ReadSize, &BytesRead);
        TotalSize -= BytesRead;
        CurOffset += BytesRead;
        if (NtRet != STATUS_SUCCESS) break;
        if (BytesRead == 0) break;
    }

    *read = CurOffset;
    return NtRet;
}

NTSTATUS WriteProcessMemory(int pid, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written) {
    PEPROCESS pProcess = NULL;
    if (pid == 0) return STATUS_UNSUCCESSFUL;

    NTSTATUS NtRet = PsLookupProcessByProcessId(pid, &pProcess);
    if (NtRet != STATUS_SUCCESS) return NtRet;

    ULONG_PTR process_dirbase = GetProcessCr3(pProcess);
    ObDereferenceObject(pProcess);

    SIZE_T CurOffset = 0;
    SIZE_T TotalSize = size;
    while (TotalSize) {
        UINT64 CurPhysAddr = TranslateLinearAddress(process_dirbase, (ULONG64)Address + CurOffset);
        if (!CurPhysAddr) return STATUS_UNSUCCESSFUL;

        ULONG64 WriteSize = min(PAGE_SIZE - (CurPhysAddr & 0xFFF), TotalSize);
        SIZE_T BytesWritten = 0;
        NtRet = WritePhysicalAddress(CurPhysAddr, (PVOID)((ULONG64)AllocatedBuffer + CurOffset), WriteSize,
                                     &BytesWritten);
        TotalSize -= BytesWritten;
        CurOffset += BytesWritten;
        if (NtRet != STATUS_SUCCESS) break;
        if (BytesWritten == 0) break;
    }

    *written = CurOffset;
    return NtRet;
}

/*
NTSTATUS ke_read_process_memory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size) {
    PEPROCESS SourceProcess = Process;
    PEPROCESS TargetProcess = PsGetCurrentProcess();
    SIZE_T Result;
    if (NT_SUCCESS(
        MmCopyVirtualMemory(SourceProcess, SourceAddress, TargetProcess, TargetAddress, Size, KernelMode, &Result)))
        return STATUS_SUCCESS;
    else
        return STATUS_ACCESS_DENIED;
}
*/
