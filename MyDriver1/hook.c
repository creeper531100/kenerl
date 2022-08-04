#include "hook.h"

PVOID* routine_ret_address;
BYTE origin_shell[13];

BOOL ke_call_function(PVOID fn_address) {
    DbgPrint("address=%p\r\n", fn_address);
    if (fn_address == NULL)
        return FALSE;
    PVOID fn = (PVOID*)(get_system_module_export("\\SystemRoot\\System32\\drivers\\dxgkrnl.sys",
                                                 "NtOpenCompositionSurfaceSectionInfo"));

    DbgPrint("fn=%p\r\n", fn);
    if (!fn) return FALSE;

    BYTE orig[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    BYTE shell_code[] = {0x48, 0xB8}; //move rax ，易被檢測
    BYTE shell_code_end[] = {0xFF, 0xE0, 0xC3}; //jmp rax ret

    RtlSecureZeroMemory(&orig, sizeof(orig));

    UINT_PTR hook_address = (UINT_PTR)(fn_address);

    //填充orig 第一次複製shell_code到orig，第二次把地址複製到orig，第三次結尾
    memcpy((PVOID)((ULONG_PTR)orig), &shell_code, sizeof(shell_code));
    memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code)), &hook_address, sizeof(PVOID));
    memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code) + sizeof(PVOID)), &shell_code_end, sizeof(shell_code_end));

    int offset;
    const BYTE* byte_ptr = (const BYTE*)fn;
    for (offset = 0; offset < 0x200; offset++) {
        if (byte_ptr[offset] == 0xC3 && byte_ptr[offset + 1] == 0xCC && byte_ptr[offset + 2] == 0xCC) //ret int3 int3
            break;
    }

    if (offset == 0x200)
        return;

    routine_ret_address = (PVOID*)(byte_ptr + offset);
    memcpy(origin_shell, routine_ret_address, sizeof(orig));

    DbgPrintEx(0, 0, "\r\n[%s] Position %p \r\n", __FUNCTION__, routine_ret_address);
    ke_write_to_read_memory(routine_ret_address, &orig, sizeof(orig));
    return TRUE;
}

BOOL ReqBase(RtlStruct* rtl_struct) {
    UNICODE_STRING name;
    RtlInitUnicodeString(&name, rtl_struct->mod_name);
    rtl_struct->base_address = GetProcessBaseAddress(rtl_struct->pid);
    return STATUS_SUCCESS;
}

BOOL Read(RtlStruct* rtl_struct) {
    ReadProcessMemory(rtl_struct->pid, rtl_struct->address, rtl_struct->out, rtl_struct->size, rtl_struct->read);
    return STATUS_SUCCESS;
}

BOOL Write(RtlStruct* rtl_struct) {
    WriteProcessMemory(rtl_struct->pid, rtl_struct->address, rtl_struct->out, rtl_struct->size, rtl_struct->read);
    return STATUS_SUCCESS;
}

BOOL Unhook(RtlStruct* rtl_struct) {
    DbgPrintEx(0, 0, "Unhook \r\n");
    ke_write_to_read_memory(routine_ret_address, &origin_shell, sizeof(origin_shell));
    return STATUS_SUCCESS;
}

NTSTATUS hook_handle(PVOID _no_param1, PVOID data, PVOID has_hook, PVOID _no_param3) {
    if (*(INT32*)has_hook != 67) {
        DbgPrintEx(0, 0, "unfriendly :( =%d\r\n", *(INT32*)has_hook);
        return STATUS_INVALID_PARAMETER;
    }
    RtlStruct* rtl_struct = (RtlStruct*)data;
    DbgPrintEx(0, 0, "select = %d\r\n", rtl_struct->io_mode);
    BOOL (*fn_array[4])(RtlStruct*) = {Write, Read, ReqBase, Unhook};
    return fn_array[rtl_struct->io_mode](rtl_struct);
}
