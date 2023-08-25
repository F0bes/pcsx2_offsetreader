// Hi there, this tool is to give an example of how to read the current recompiler base offset at runtime
// Due to how our (pcsx2) recompiler memory is managed, we are no longer able to ensure that a static address will be available
// Make sure this process is the same bitness as pcsx2

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <Psapi.h>
#include "GetProcAddressEx.h"


int main(int argc, char* argv)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	bool found_process = false;

	HANDLE ss = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(ss, &entry) == TRUE)
	{
		// Enumerate through the snapshot, looking for the PCSX2 process
		while (Process32Next(ss, &entry) == TRUE)
		{
			// This executable name can and will change depending on devel builds, retail builds, or if users rename it
			if (_wcsicmp(entry.szExeFile, L"pcsx2-qtx64-avx2-dev.exe") == 0)
			{
				found_process = true;
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				if (hProcess == NULL)
				{
					std::cout << "OpenProcess Failed. GetLastError: " << GetLastError();
					return -1;
				}

				std::cout << "PCSX2 process found" << std::endl;

				HMODULE hModule[1024];
				DWORD hModuleSizeNeeded;
				// We have successfully retrieved a handle to PCSX2. Fetch all of the modules loaded in PCSX2
				if (!EnumProcessModules(hProcess, hModule, sizeof(hModule), &hModuleSizeNeeded))
				{
					std::cout << "EnumProcessModules GetLastError: " << GetLastError();
					return -1;
				}
				if (hModuleSizeNeeded > sizeof(hModule))
				{
					std::cout << "hModule array too small, try increasing it from " << sizeof(hModule) / sizeof(HMODULE) << std::endl;
					return -1;
				}
				DWORD modulesFound = hModuleSizeNeeded / sizeof(HMODULE);
				std::cout << "Found " << modulesFound << " modules\n";

				// This assumes that the first module in PCSX2 will be PCSX2 itself
				// I'm unsure if this will ever _not_ be the case
				HMODULE hPCSX2 = hModule[0];

				// GetProcAddressEx does symbol lookup for us, returning the address where that symbol is located
				// These addresses are pointers! These are not the base address values
				PVOID EEmemAddress = GetProcAddressEx(hProcess, hPCSX2, "EEmem");
				PVOID IOPmemAddress = GetProcAddressEx(hProcess, hPCSX2, "IOPmem");
				PVOID VUmemAddress = GetProcAddressEx(hProcess, hPCSX2, "VUmem");

				// These will hold our actual base addresses we can use with WriteProcessMemory and ReadProcessMemory
				uintptr_t EEmemBaseAddress;
				uintptr_t IOPmemBaseAddress;
				uintptr_t VUmemBaseAddress;

				SIZE_T bytesRead;
				// We need to dereference the pointers to get that actual starting address of our memory segments
				ReadProcessMemory(hProcess, EEmemAddress, &EEmemBaseAddress, sizeof(uintptr_t), &bytesRead);
				ReadProcessMemory(hProcess, IOPmemAddress, &IOPmemBaseAddress, sizeof(uintptr_t), &bytesRead);
				ReadProcessMemory(hProcess, VUmemAddress, &VUmemBaseAddress, sizeof(uintptr_t), &bytesRead);

				std::cout << std::hex << "EEmem:  " << (uintptr_t)EEmemAddress << "->" << EEmemBaseAddress << "\n";
				std::cout << std::hex << "IOPmem: " << (uintptr_t)IOPmemAddress << "->" << IOPmemBaseAddress << "\n";
				std::cout << std::hex << "VUmem: " << (uintptr_t)VUmemAddress << "->" << VUmemBaseAddress << "\n";

				// Now that we have our base addresses, let's look for a string that starts with "sce" in our EE memory region
				const char* stringToFind = "sce";
				for (int i = 0x200000; i < 0x300000; i++)
				{
					char rpmBuffer[3];
					SIZE_T bytesRead;
					uintptr_t addressToRead = EEmemBaseAddress + i;
					ReadProcessMemory(hProcess, (PVOID)addressToRead, rpmBuffer, 3, &bytesRead);

					if (memcmp(rpmBuffer, stringToFind, 3) == 0)
					{
						std::cout << "Found instance of \"sce\" at address " << addressToRead << "\n";
						char stringBuffer[256];
						ReadProcessMemory(hProcess, (PVOID)addressToRead, stringBuffer, sizeof(stringBuffer), &bytesRead);

						std::cout << "Full string: " << stringBuffer << std::endl;
					}
				}
				// Cleanup after we are done
				CloseHandle(hProcess);
			}
		}

		if (!found_process)
		{
			std::cout << "Couldn't find the PCSX2 process" << std::endl;
		}
	}
	return 0;
}