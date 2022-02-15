// Hi there, this tool is to give an example of how to read the current recompiler base offset at runtime
// Due to how our (pcsx2) recompiler memory is managed, we are no longer able to ensure that a static address will be available
// Make sure this process is the same bitness as pcsx2

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <Psapi.h>
#include "GetProcAddressEx.h"


int main(void)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE ss = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	
	if (Process32First(ss, &entry) == TRUE)
	{
		while (Process32Next(ss, &entry) == TRUE)
		{
			if (_wcsicmp(entry.szExeFile, L"pcsx2x64-avx2-dev.exe") == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				if (hProcess == NULL)
				{
					std::cout << "OpenProcess Failed --> ", GetLastError();
					return -1;
				}

				printf("Found pcsx2 process :)\n");
				
				HMODULE hModule[1024];
				DWORD hModuleSizeNeeded;
				if (!EnumProcessModules(hProcess, hModule, sizeof(hModule), &hModuleSizeNeeded))
				{
					std::cout << "EnumProcessModules Failed --> " << GetLastError();
					return -1;
				}
				if (hModuleSizeNeeded > sizeof(hModule))
				{
					std::cout << "hModule too small, oops.\n";
					return -1;
				}
				DWORD modulesFound = hModuleSizeNeeded / sizeof(HMODULE);
				std::cout << "Found " << modulesFound << "modules\n";

				// ASSUME THAT THE EXE IS THE FIRST MODULE, THIS _SHOULD_ BE THE CASE NO?

				HMODULE hPCSX2 = hModule[0];
				PVOID EEmemAddress = GetProcAddressEx(hProcess, hPCSX2, "EEmem");
				uintptr_t baseAddress;
				SIZE_T readBytes;
				// Finally, read the EEmem variable
				ReadProcessMemory(hProcess, EEmemAddress, &baseAddress, sizeof(uintptr_t), &readBytes);

				std::cout << "EE mem address is " << std::hex << baseAddress << "\n";
				
				CloseHandle(hProcess);
			}
		}

		std::cout << "Couldn't find the PCSX2 process :(" << std::endl;
	}
	return 0;
}