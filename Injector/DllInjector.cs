using System.Runtime.InteropServices;

namespace CS2BodycamInjector;

/// <summary>
/// Классический DLL-инжектор через CreateRemoteThread + LoadLibraryW.
/// Работает только на x64 процессах (cs2.exe — 64-bit).
/// </summary>
public static class DllInjector
{
    // -------------------------------------------------------------------------
    // WinAPI imports
    // -------------------------------------------------------------------------

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern nint OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

    [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
    private static extern nint VirtualAllocEx(
        nint hProcess, nint lpAddress, nuint dwSize,
        uint flAllocationType, uint flProtect);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool WriteProcessMemory(
        nint hProcess, nint lpBaseAddress,
        byte[] lpBuffer, nuint nSize,
        out nuint lpNumberOfBytesWritten);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    private static extern nint GetProcAddress(nint hModule, string lpProcName);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern nint GetModuleHandleW(string lpModuleName);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern nint CreateRemoteThread(
        nint hProcess, nint lpThreadAttributes, nuint dwStackSize,
        nint lpStartAddress, nint lpParameter,
        uint dwCreationFlags, out uint lpThreadId);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint WaitForSingleObject(nint hHandle, uint dwMilliseconds);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool CloseHandle(nint hObject);

    // -------------------------------------------------------------------------
    // Константы
    // -------------------------------------------------------------------------

    private const uint PROCESS_ALL_ACCESS  = 0x1F0FFF;
    private const uint MEM_COMMIT          = 0x00001000;
    private const uint MEM_RESERVE         = 0x00002000;
    private const uint PAGE_READWRITE      = 0x04;
    private const uint INFINITE            = 0xFFFFFFFF;

    // -------------------------------------------------------------------------
    // Публичный API
    // -------------------------------------------------------------------------

    /// <summary>
    /// Инжектирует <paramref name="dllPath"/> в процесс <paramref name="processId"/>.
    /// Возвращает (true, "OK") или (false, "описание ошибки").
    /// </summary>
    public static (bool Success, string Message) Inject(int processId, string dllPath)
    {
        // Путь должен быть абсолютным — LoadLibraryW грузит относительные пути
        // из контекста ЦЕЛЕВОГО процесса, а не нашего.
        dllPath = Path.GetFullPath(dllPath);

        nint hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
        if (hProcess == nint.Zero)
            return (false, $"OpenProcess failed (Win32={Marshal.GetLastWin32Error()})");

        try
        {
            // Записываем строку-путь в память целевого процесса (Unicode, null-terminated)
            byte[] pathBytes = System.Text.Encoding.Unicode.GetBytes(dllPath + '\0');

            nint remoteBuffer = VirtualAllocEx(
                hProcess, nint.Zero, (nuint)pathBytes.Length,
                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

            if (remoteBuffer == nint.Zero)
                return (false, $"VirtualAllocEx failed (Win32={Marshal.GetLastWin32Error()})");

            WriteProcessMemory(hProcess, remoteBuffer, pathBytes, (nuint)pathBytes.Length, out _);

            // LoadLibraryW находится в kernel32.dll, которая одинаково смаплена во всех x64 процессах
            nint hKernel32   = GetModuleHandleW("kernel32.dll");
            nint loadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");

            if (loadLibraryW == nint.Zero)
                return (false, "GetProcAddress(LoadLibraryW) failed — очень странно");

            nint hThread = CreateRemoteThread(
                hProcess, nint.Zero, 0,
                loadLibraryW, remoteBuffer,
                0, out _);

            if (hThread == nint.Zero)
                return (false, $"CreateRemoteThread failed (Win32={Marshal.GetLastWin32Error()})");

            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);

            return (true, $"DLL успешно загружена в PID {processId}");
        }
        finally
        {
            CloseHandle(hProcess);
        }
    }

    /// <summary>
    /// Извлекает вшитый DLL-ресурс во временную папку и возвращает путь к нему.
    /// </summary>
    public static string ExtractEmbeddedDll(string resourceName)
    {
        var assembly = System.Reflection.Assembly.GetExecutingAssembly();
        using var stream = assembly.GetManifestResourceStream(resourceName)
            ?? throw new InvalidOperationException(
                $"Ресурс '{resourceName}' не найден в сборке.\n" +
                $"Доступные: {string.Join(", ", assembly.GetManifestResourceNames())}");

        string outPath = Path.Combine(Path.GetTempPath(), "bodycam_mod.dll");
        using var fs = new FileStream(outPath, FileMode.Create, FileAccess.Write, FileShare.None);
        stream.CopyTo(fs);
        return outPath;
    }
}
