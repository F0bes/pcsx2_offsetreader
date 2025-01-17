// Hi there, this tool is to give an example of how to read the current recompiler base offset at runtime
// Due to how our (pcsx2) recompiler memory is managed, we are no longer able to ensure that a static address will be available
// Make sure this process is the same bitness as pcsx2

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <Psapi.h>

bool GetProcAddressEx(const HANDLE& hProc, const __int64& dwModule, const std::string& fnName, __int64* lpResult);

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
			if (_wcsicmp(entry.szExeFile, L"pcsx2-qt.exe") == 0)
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
				__int64 EEmemAddress;
				GetProcAddressEx(hProcess, __int64(hPCSX2), "EEmem", &EEmemAddress);

				__int64 IOPmemAddress;
				GetProcAddressEx(hProcess, __int64(hPCSX2), "IOPmem", &IOPmemAddress);

				__int64 VUmemAddress;
				GetProcAddressEx(hProcess, __int64(hPCSX2), "VUmem", &VUmemAddress);

				// These will hold our actual base addresses we can use with WriteProcessMemory and ReadProcessMemory
				uintptr_t EEmemBaseAddress;
				uintptr_t IOPmemBaseAddress;
				uintptr_t VUmemBaseAddress;

				SIZE_T bytesRead;
				// We need to dereference the pointers to get that actual starting address of our memory segments
				ReadProcessMemory(hProcess, LPVOID(EEmemAddress), &EEmemBaseAddress, sizeof(uintptr_t), &bytesRead);
				ReadProcessMemory(hProcess, LPVOID(IOPmemAddress), &IOPmemBaseAddress, sizeof(uintptr_t), &bytesRead);
				ReadProcessMemory(hProcess, LPVOID(VUmemAddress), &VUmemBaseAddress, sizeof(uintptr_t), &bytesRead);

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

bool GetProcAddressEx(const HANDLE& hProc, const __int64& dwModule, const std::string& fnName, __int64* lpResult)
{
	//  fn transform a wide character string into a string
	static auto ToString = [](const std::wstring& input) -> std::string
		{
			return std::string(input.begin(), input.end());
		};

	//	fn to transform an input string to lowercase
	static auto ToLower = [](const std::string& input) -> std::string
		{
			std::string result;
			for (auto c : input)
				result += tolower(c);
			return result;
		};

	//	fn to read memory in the target process at the specified address
	static auto ReadMemory = [](const HANDLE& hProc, const __int64& addr, void* lpResult, size_t szRead) -> bool
		{
			SIZE_T size_read{};
			return ReadProcessMemory(hProc, LPCVOID(addr), lpResult, szRead, &size_read) && szRead == size_read;
		};

	//	fn to read a continguous string in the target process at the specified address
	static auto ReadString = [](const HANDLE& hProc, const __int64& addr, const size_t& szString, std::string* lpResult) -> bool
		{
			size_t bytes_read{};
			char buf[MAX_PATH]{};
			if (!ReadMemory(hProc, addr, buf, szString))
				return false;

			*lpResult = std::string(buf);

			return true;
		};


	//	transform input to lowerstring for later comparison
	const auto& fnNameLower = ToLower(fnName);

	//	get image doe header
	IMAGE_DOS_HEADER image_dos_header;
	if (!ReadMemory(hProc, dwModule, &image_dos_header, sizeof(image_dos_header)) 
		|| image_dos_header.e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	//	get nt headers
	IMAGE_NT_HEADERS image_nt_headers;
	if (!ReadMemory(hProc, dwModule + image_dos_header.e_lfanew, &image_nt_headers, sizeof(image_nt_headers)) 
		|| image_nt_headers.Signature != IMAGE_NT_SIGNATURE 
		|| image_nt_headers.OptionalHeader.NumberOfRvaAndSizes <= 0)
		return false;

	//	get export directory
	const auto& export_directory_va = image_nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + dwModule;
	IMAGE_EXPORT_DIRECTORY export_directory;
	if (!ReadMemory(hProc, export_directory_va, &export_directory, sizeof(export_directory)) 
		|| !export_directory.AddressOfNames 
		|| !export_directory.AddressOfFunctions 
		|| !export_directory.AddressOfNameOrdinals)
		return false;

	//	get address of *
	const auto& names_va = dwModule + export_directory.AddressOfNames;
	const auto& functions_va = dwModule + export_directory.AddressOfFunctions;
	const auto& ordinals_va = dwModule + export_directory.AddressOfNameOrdinals;
	for (int i = 0; i < export_directory.NumberOfNames; i++)
	{
		//	get address of name
		DWORD name_rva;
		if (!ReadMemory(hProc, __int64(names_va + (i * sizeof(name_rva))), &name_rva, sizeof(name_rva)))
			continue;

		const auto& name_va = name_rva + dwModule;

		//	read & compare name with input string
		std::string cmp;
		if (!ReadString(hProc, name_va, MAX_PATH, &cmp))
			continue;

		//	compare strings
		if (fnNameLower != ToLower(cmp))
			continue;

		//	get function address
		short name_ordinal;
		if (!ReadMemory(hProc, __int64(ordinals_va + (i * sizeof(name_ordinal))), &name_ordinal, sizeof(name_ordinal)))	//	get ordinal at the current index
			return false;

		DWORD function_rva;
		if (!ReadMemory(hProc, __int64(functions_va + (name_ordinal * sizeof(function_rva))), &function_rva, sizeof(function_rva)))	//	get function va from the ordinal index of the functions array
			return false;

		//	pass result
		*lpResult = __int64(function_rva + dwModule);

		return true;
	}

	return false;
}