# pcsx2_offsetreader

Test program for my PR that exports the recompilers base address for use by third party tools

Thanks to https://github.com/Mac-A-4/GetProcAddressEx for the missing GetProcAddressEx function

Note: If you don't want to depend on an implementation of GetProcAddressEx you can inject a DLL into PCSX2 and then use GetProcAddress

