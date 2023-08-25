# pcsx2_offsetreader

Test program for my PR that exports the recompilers base address for use by third party tools

Thanks to https://github.com/Mac-A-4/GetProcAddressEx for the missing GetProcAddressEx function

For windows there are examples for external and internal methods of reading/writing memory

The Linux example is only from an external process. I'm unsure if an "internal" method could exist for linux

## Windows examples
### External 

This method takes a little bit more boilerplate

ReadProcessMemory and WriteProcessMemory is used to access the guest memory map in PCSX2

**I'd recommend using this method** and writing a wrapper class around the wpm/rpm functions

Here is a snippet of a wrapper around rpm
```cpp
template<typename T>
T pcsx2Read(uintptr_t vmAddress)
{
	T buffer;
	SIZE_T bytesRead;

	if (!ReadProcessMemory(pcsx2Handle, reinterpret_cast<LPCVOID>(vmAddress), &buffer, sizeof(T), &bytesRead)) {
		throw std::runtime_error("ReadProcessMemory failed");
	}

	if (bytesRead != sizeof(T)) {
		throw std::runtime_error("ReadProcessMemory did not read the expected number of bytes");
	}

	return buffer;
}

// Call this function by doing
uint32_t player_health = pcsx2Read<uint32_t>(health_offset);

// Or with non-POD types
playerInfo = pcsx2Read<typeof(playerInfo)>(player_offset);
```

### Internal
This method is a little bit easier to write for, but requires an external program to inject your library (dll) into PCSX2

This method can directly access the guest memory map in PCSX2 without the need for ReadProcessMemory and WriteProcessMemory


