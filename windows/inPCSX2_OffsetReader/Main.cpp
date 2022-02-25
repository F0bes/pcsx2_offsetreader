#include <windows.h>
#include <iostream>

bool INFO = FALSE;

namespace PCSX2 {
    HMODULE module = NULL;
    uintptr_t BaseAddress = NULL;
    uintptr_t EEmem = NULL;
}

namespace Offsets {
    int RENDERFIX = 0x33CD68;   //  2033CD68
}

void displayOptions()
{
    system("cls");
    std::cout << R"(--------------------------------------
EXAMPLE GAME OPTIONS: 
*SOCOM 2 NTSC MUST BE RUNNING

[1] GAME Info
[2] Patch Render Fix
[END] QUIT
--------------------------------------

)";
    INFO = TRUE;
}

DWORD WINAPI MainThread(HMODULE hModule)
{
    //  OPEN CONSOLE
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    //  ESTABLISH PROC INFO
    PCSX2::module = GetModuleHandleA(NULL);
    PCSX2::BaseAddress = (uintptr_t)PCSX2::module;
    PCSX2::EEmem = *(uintptr_t*)GetProcAddress(PCSX2::module, "EEmem");

    //  CONSOLE DEBUG
    printf("PROCESS INFORMATION: \n");
    printf("PCSX2 Base Address: %llX\n", PCSX2::BaseAddress);
    printf("PCSX2 EEmem BaseAddress: %llX\n\n", PCSX2::EEmem);
    printf("[END] QUIT\n");
    printf("[INSERT] Example Options\n");

    bool dosomething = true;
    while (dosomething) {

        //  HOTKEY TO EXIT
        if (GetAsyncKeyState(VK_END) & 1) break;

        if (GetAsyncKeyState(VK_INSERT) & 1)
            if (!INFO)
                displayOptions();

        if (INFO) {

            //  EXAMPLE READ
            if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
                uintptr_t RENDERaddr = (PCSX2::EEmem + Offsets::RENDERFIX);
                uintptr_t RENDERvalue = *(int*)(RENDERaddr);
                printf("--GAME INFORMATION\n");
                printf("Render Fix Address: %llX\n", RENDERaddr);
                printf("Render Fix Value: %X\n", RENDERvalue);
            }

            //  EXAMPLE WRITE
            if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
                system("cls");
                uintptr_t RENDERaddr = (PCSX2::EEmem + Offsets::RENDERFIX);
                uintptr_t RENDERvalue = *(int*)(RENDERaddr);
                printf("PATCHING . . .\n");
                Sleep(1000);
                if (RENDERvalue != (int)0x100000DB)
                    *(int*)RENDERaddr = (int)0x100000DB;
                else {
                    printf("ALREADY PATCHED!");
                    Sleep(1500);
                }
                dosomething = false;
            }
        }
    }

    //  EXIT
    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr));
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}