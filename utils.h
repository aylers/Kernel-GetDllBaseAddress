extern "C" 	__declspec(dllimport) PPEB PsGetProcessPeb(PEPROCESS);

typedef struct _PEB_LDR_DATA PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _PEB {
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA Ldr;
} PEB, * PPEB;


typedef struct _PEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, * PPEB_LDR_DATA;


ULONG64 GetModuleBasex64(PEPROCESS proc, UNICODE_STRING module_name, bool get_size) {
	PPEB pPeb = (PPEB)PsGetProcessPeb(proc); // get Process PEB, function is unexported and undoc

	if (!pPeb) {
		return 0; // failed
	}

	KAPC_STATE state;

	KeStackAttachProcess(proc, &state);

	PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;

	if (!pLdr) {
		KeUnstackDetachProcess(&state);
		return 0; // failed
	}

	UNICODE_STRING name;

	// loop the linked list
	for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->InLoadOrderModuleList.Flink;
		list != &pLdr->InLoadOrderModuleList; list = (PLIST_ENTRY)list->Flink)
	{
		PLDR_DATA_TABLE_ENTRY pEntry =
			CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		DbgPrint("Module Name: %wZ\n", pEntry->BaseDllName);
		DbgPrint("Module Base: %p\n", pEntry->DllBase);
		DbgPrint("Module Size: %d\n", pEntry->SizeOfImage);

		if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) ==
			0) {
			ULONG64 baseAddr = (ULONG64)pEntry->DllBase;
			ULONG64 moduleSize = (ULONG64)pEntry->SizeOfImage; // get the size of the module
			KeUnstackDetachProcess(&state);
			if (get_size) {
				return moduleSize; // return the size of the module if get_size is TRUE
			}
			return baseAddr;
		}
	}

	KeUnstackDetachProcess(&state);

	return 0; // failed
}

// IOCTL Command
if (code == get_client_module) {
		if (size == sizeof(MODULE_REQUEST)) {
			PMODULE_REQUEST req = (PMODULE_REQUEST)(Irp->AssociatedIrp.SystemBuffer);

			PEPROCESS Process = NULL;
			UNICODE_STRING module_name;

			if (NT_SUCCESS(PsLookupProcessByProcessId(req->ProcessId, &Process))) {

				// Convert req->ModuleName to UNICODE_STRING
				RtlInitUnicodeString(&module_name, req->ModuleName);

				// Get the base address of the requested module
				req->ModuleBase = (PVOID)GetModuleBasex64(Process, module_name, false);

				if (req->ModuleBase != NULL) {
					status = STATUS_SUCCESS;
					bytes = sizeof(MODULE_REQUEST);
				}
				else {
					status = STATUS_NOT_FOUND; // If the module wasn't found
				}

				// Dereference the process object
				ObDereferenceObject(Process);
			}
			else {
				status = STATUS_INVALID_PARAMETER; // Failed to find the process
			}
		}
		else {
			status = STATUS_INFO_LENGTH_MISMATCH;
			bytes = 0;
		}
	}
