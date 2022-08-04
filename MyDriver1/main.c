#include "hook.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING);

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    DbgPrint(("Inject\r\n"));

    ke_call_function(&hook_handle);

    return STATUS_SUCCESS;
}
