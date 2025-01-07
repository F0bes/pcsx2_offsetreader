using System;
using System.Runtime.InteropServices;

namespace pcsx2_offsetreader_client_csharp
{
    internal class Program
    {

        [DllImport("pcsx2_offsetreader_external.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool SetupExternalWrapper(string processName);

        [DllImport("pcsx2_offsetreader_external.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CleanupExternalWrapper();

        [DllImport("pcsx2_offsetreader_external.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern byte ReadMemory8(uint ps2_addr);

        [DllImport("pcsx2_offsetreader_external.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WriteMemory8(uint ps2_addr, byte value);

        static void Main(string[] args)
        {
            Console.WriteLine("Setting up external wrapper...");
            if (!SetupExternalWrapper("pcsx2-qt.exe"))
            {
                Console.WriteLine("Failed to setup external wrapper.");
                return;
            }

            byte val = ReadMemory8(0x42BA54);
            // print as hex
            Console.WriteLine("Value at 0x042C61C: 0x" + val.ToString());
            Console.WriteLine("Scanning memory from 0x100000 to 0x200000 for the string \"Cache\"");

            for (uint i = 0x100000; i < 0x200000; i++)
            {
                if (ReadMemory8(i) == 'C' && ReadMemory8(i + 1) == 'a' && ReadMemory8(i + 2) == 'c' && ReadMemory8(i + 3) == 'h' && ReadMemory8(i + 4) == 'e')
                {
                    Console.WriteLine("Found string \"Cache\" at 0x" + i.ToString("X"));
                }
            }

            WriteMemory8(0x200000, 0xB0);
            WriteMemory8(0x200001, 0x00);
            WriteMemory8(0x200002, 0xBA);

            CleanupExternalWrapper();
        }
    }
}
