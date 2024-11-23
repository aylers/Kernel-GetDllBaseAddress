typedef struct _MODULE_REQUEST {
	HANDLE ProcessId;         
	WCHAR ModuleName[260];    
	PVOID ModuleBase;         
} MODULE_REQUEST, * PMODULE_REQUEST;



uintptr_t get_module_base(std::string dll_name) {
	uintptr_t image_address = { NULL };
	_MODULE_REQUEST arguments = { NULL };

	

	mbstowcs(arguments.ModuleName, dll_name.c_str(), 260);

	arguments.ProcessId = (HANDLE)process_id;
	arguments.ModuleBase = (PVOID)image_address;
	BOOL result = DeviceIoControl(
		driver_handle,
		get_client_module,
		&arguments,
		sizeof(arguments),
		&arguments,
		sizeof(arguments),
		nullptr,
		nullptr
	);

	if (result && arguments.ModuleBase != nullptr) {
		image_address = reinterpret_cast<uintptr_t>(arguments.ModuleBase);
	}
	else {
		wprintf(L"DeviceIoControl failed with error: %lu\n", GetLastError());
	}

}

// how to call the function
driver::get_module_base("client.dll");
