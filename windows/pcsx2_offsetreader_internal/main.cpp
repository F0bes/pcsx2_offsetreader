#include <windows.h>
#include <iostream>

DWORD WINAPI MainThread(HMODULE hinstDLL)
{
	// Get our current process handle
	HMODULE hPCSX2 = GetModuleHandleA(NULL);

	// Because we live inside of the process, we can simply dereference what GetProcAddress returns
	// Otherwise we would have to use ReadProcessMemory
	uintptr_t EEmemBaseAddress = *(uintptr_t*)GetProcAddress(hPCSX2, "EEmem");
	uintptr_t IOPmemBaseAddress = *(uintptr_t*)GetProcAddress(hPCSX2, "IOPmem");
	uintptr_t VUmemBaseAddress = *(uintptr_t*)GetProcAddress(hPCSX2, "VUmem");

	// This will print to the already allocated console 
	std::cout << "pcsx2_offsetreader_internal mainthread" << std::endl << std::hex;
	std::cout << "EEmem:  " << EEmemBaseAddress << "\n";
	std::cout << "IOPmem: " << "->" << IOPmemBaseAddress << "\n";
	std::cout << "VUmem: " << "->" << VUmemBaseAddress << "\n";

	// Now that we have our base addresses, let's look for a string that starts with "sce" in our EE memory region
	const char* stringToFind = "sce";
	for (int i = 0x200000; i < 0x300000; i++)
	{
		char* strBuffer = reinterpret_cast<char*>(EEmemBaseAddress + i);
		if (memcmp(strBuffer, stringToFind, 3) == 0)
		{
			std::cout << "Found instance of \"sce\" at address " << EEmemBaseAddress + i << "\n";

			// Kind of dangerous, if we happen to come across something that isn't actually a string,
			// it'll read until the first null terminator (which could be into invalid memory!)
			// But this is just for an example
			std::cout << "Full string: " << strBuffer << std::endl;
		}
	}

	FreeLibraryAndExitThread(hinstDLL, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hinstDLL, DWORD  fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hinstDLL, 0, nullptr));
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
