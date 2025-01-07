// Hi there, this tool is to give an example of how to read the current recompiler base offset at runtime
// Due to how our (pcsx2) recompiler memory is managed, we are no longer able to ensure that a static address will be available
// Make sure this process is the same bitness as pcsx2

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <Psapi.h>
#include "GetProcAddressEx.h"

extern "C" {
	__declspec(dllexport) bool SetupExternalWrapper(const char* processName);
	__declspec(dllexport) void CleanupExternalWrapper();
	__declspec(dllexport) uint8_t ReadMemory8(uint32_t ps2_addr);
	__declspec(dllexport) void WriteMemory8(uint32_t ps2_addr, uint8_t value);
}

static HANDLE snapshot = INVALID_HANDLE_VALUE;
static HANDLE hProcess = INVALID_HANDLE_VALUE;

// These will hold our actual base addresses we can use with WriteProcessMemory and ReadProcessMemory
static uintptr_t EEmemBaseAddress;
static uintptr_t IOPmemBaseAddress;
static uintptr_t VUmemBaseAddress;

bool SetupExternalWrapper(const char* processName)
{
	std::cout << "[dll] SetupExternalWrapper()\n";
	if (hProcess != INVALID_HANDLE_VALUE || snapshot != INVALID_HANDLE_VALUE)
	{
		std::cerr << "Already setup!\n";
		return false;
	}

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	bool found_process = false;

	HANDLE ss = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(ss, &entry) == TRUE)
	{
		std::unique_ptr<wchar_t[]> wProcessName = std::make_unique<wchar_t[]>(strlen(processName) + 1);
		mbstowcs_s(NULL, wProcessName.get(), strlen(processName) + 1, processName, strlen(processName));
		
		std::wcout << "Looking for process: \"" << wProcessName.get() << "\"" << std::endl;
		// Enumerate through the snapshot, looking for the PCSX2 process
		while (Process32Next(ss, &entry) == TRUE)
		{
			if (_wcsicmp(entry.szExeFile, wProcessName.get()) == 0)
			{
				found_process = true;
				hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				if (hProcess == NULL)
				{
					std::cerr << "OpenProcess Failed. GetLastError: " << GetLastError() << std::endl;
					hProcess == INVALID_HANDLE_VALUE;
					return false;
				}

				std::cout << "PCSX2 process found" << std::endl;

				HMODULE hModule[1024];
				DWORD hModuleSizeNeeded;
				// We have successfully retrieved a handle to PCSX2. Fetch all of the modules loaded in PCSX2
				if (!EnumProcessModules(hProcess, hModule, sizeof(hModule), &hModuleSizeNeeded))
				{
					std::cerr << "EnumProcessModules GetLastError: " << GetLastError();
					return false;
				}
				if (hModuleSizeNeeded > sizeof(hModule))
				{
					std::cerr << "hModule array too small, try increasing it from " << sizeof(hModule) / sizeof(HMODULE) << std::endl;
					return false;
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

				SIZE_T bytesRead;
				// We need to dereference the pointers to get that actual starting address of our memory segments
				ReadProcessMemory(hProcess, EEmemAddress, &EEmemBaseAddress, sizeof(uintptr_t), &bytesRead);
				ReadProcessMemory(hProcess, IOPmemAddress, &IOPmemBaseAddress, sizeof(uintptr_t), &bytesRead);
				ReadProcessMemory(hProcess, VUmemAddress, &VUmemBaseAddress, sizeof(uintptr_t), &bytesRead);

				std::cout << std::hex << "EEmem:  " << (uintptr_t)EEmemAddress << "->" << EEmemBaseAddress << "\n";
				std::cout << std::hex << "IOPmem: " << (uintptr_t)IOPmemAddress << "->" << IOPmemBaseAddress << "\n";
				std::cout << std::hex << "VUmem: " << (uintptr_t)VUmemAddress << "->" << VUmemBaseAddress << "\n";
				return true;
			}
		}

		if (!found_process)
		{
			std::cerr << "Couldn't find the PCSX2 process" << std::endl;
		}
	}
	return false;
}

void CleanupExternalWrapper()
{
	if (hProcess != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hProcess);
		hProcess = INVALID_HANDLE_VALUE;
	}
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		CloseHandle(snapshot);
		snapshot = INVALID_HANDLE_VALUE;
	}
}

uint8_t ReadMemory8(uint32_t ps2_addr)
{
	uintptr_t addressToRead = EEmemBaseAddress + ps2_addr;
	uint8_t byte;
	SIZE_T bytesRead;
	ReadProcessMemory(hProcess, (PVOID)addressToRead, &byte, 1, &bytesRead);
	return byte;
}

void WriteMemory8(uint32_t ps2_addr, uint8_t value)
{
	uintptr_t addressToWrite = EEmemBaseAddress + ps2_addr;
	SIZE_T bytesWritten;
	WriteProcessMemory(hProcess, (PVOID)addressToWrite, &value, 1, &bytesWritten);
}