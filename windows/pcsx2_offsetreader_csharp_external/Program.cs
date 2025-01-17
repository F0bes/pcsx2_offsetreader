using pcsx2_offsetreader_csharp_external;

//  Attach to PCSX2 Process
//  - obtains PID, Opens handle with all access & retrieves the ModuleBase Address
if (Memory.Attach("pcsx2-qt", Memory.ProcessAccessFlags.All))
{
    //  log info
    Console.WriteLine($"PID:\t\t{Memory.ProcID}\nBASE:\t\t0x{Memory.ProcModBase:X}");

    //  Get the reltative virtual address of exported variable "EEmem"
    IntPtr pEEmem = Memory.GetProcAddress("EEmem");
    if (pEEmem != IntPtr.Zero)
    {
        //  log base address and EE mem address
        var PS2Base = Memory.ReadMemory<IntPtr>(pEEmem);
        Console.WriteLine($"EE:\t\t0x{pEEmem:X}\nPS2BASE:\t0x{PS2Base:X}");
        if (PS2Base != IntPtr.Zero)
        {
            //  Read PS2 Memory : default reads base address
            Int32 offset = 0x0;
            IntPtr address = PS2Base + offset;
            Int32 readBytes = Memory.ReadMemory<Int32>(address);
            Console.WriteLine($"read {sizeof(Int32)} bytes at 0x{address:X} -> 0x{readBytes:X}");
        }
    }
}
else
    Console.WriteLine("Failed to attach to process 'pcsx2-qt'");

Console.WriteLine("press [enter] to exit");
Console.ReadLine(); //  enter to exit