using System.Diagnostics;

namespace CS2BodycamInjector;

public partial class MainForm : Form
{
    // -----------------------------------------------------------------------
    // Конфиг — поправь путь если Steam стоит не на C:
    // -----------------------------------------------------------------------
    private const string CS2_PROCESS_NAME = "cs2";

    private static readonly string CS2_EXE = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
        @"Steam\steamapps\common\Counter-Strike Global Offensive\game\bin\win64\cs2.exe");

    // Имя embedded-ресурса: {RootNamespace}.{папка}.{файл}
    // (точки вместо слешей, именно так .NET называет вложенные ресурсы)
    private const string DLL_RESOURCE = "CS2BodycamInjector.Resources.bodycam_mod.dll";

    // -----------------------------------------------------------------------

    public MainForm()
    {
        InitializeComponent();
        CheckDllEmbedded();
    }

    // Предупреждаем заранее, если ресурс не вшит (собери C++ сначала)
    private void CheckDllEmbedded()
    {
        var names = System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceNames();
        bool hasDll = Array.Exists(names, n => n == DLL_RESOURCE);
        btnInject.Enabled = hasDll;

        if (!hasDll)
            Log("⚠ bodycam_mod.dll не вшит — сначала собери C++ проект, " +
                "скопируй .dll в Injector/Resources/ и пересобери injector.");
        else
            Log("✓ bodycam_mod.dll найден в ресурсах — готов к инъекции.");
    }

    // -----------------------------------------------------------------------
    // Кнопка «Запустить CS2 -insecure»
    // -----------------------------------------------------------------------
    private void btnLaunchCS2_Click(object sender, EventArgs e)
    {
        if (!File.Exists(CS2_EXE))
        {
            Log($"[ERR] cs2.exe не найден по пути:\n      {CS2_EXE}");
            Log("      Поправь CS2_EXE в MainForm.cs или укажи путь вручную.");
            return;
        }

        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName        = CS2_EXE,
                Arguments       = "-insecure",
                UseShellExecute = true,
            });
            Log($"[OK] CS2 запущен с флагом -insecure");
        }
        catch (Exception ex)
        {
            Log($"[ERR] Не удалось запустить CS2: {ex.Message}");
        }
    }

    // -----------------------------------------------------------------------
    // Кнопка «Inject»
    // -----------------------------------------------------------------------
    private async void btnInject_Click(object sender, EventArgs e)
    {
        var procs = Process.GetProcessesByName(CS2_PROCESS_NAME);
        if (procs.Length == 0)
        {
            Log("[ERR] cs2.exe не запущен. Сначала нажми «Launch CS2».");
            return;
        }

        btnInject.Enabled = false;
        btnInject.Text    = "Injecting...";

        try
        {
            await Task.Run(() =>
            {
                // 1) Извлекаем DLL из ресурсов во %TEMP%
                string dllPath = DllInjector.ExtractEmbeddedDll(DLL_RESOURCE);
                Invoke(() => Log($"[>>] DLL извлечён: {dllPath}"));

                // 2) Инжектируем
                var (ok, msg) = DllInjector.Inject(procs[0].Id, dllPath);
                Invoke(() => Log(ok ? $"[OK] {msg}" : $"[ERR] {msg}"));
            });
        }
        catch (Exception ex)
        {
            Log($"[EXC] {ex.Message}");
        }
        finally
        {
            btnInject.Enabled = true;
            btnInject.Text    = "Inject Bodycam Mod";
        }
    }

    // -----------------------------------------------------------------------
    // Хелпер
    // -----------------------------------------------------------------------
    private void Log(string text)
    {
        if (InvokeRequired) { Invoke(() => Log(text)); return; }
        listBoxLog.Items.Add($"[{DateTime.Now:HH:mm:ss}]  {text}");
        listBoxLog.TopIndex = listBoxLog.Items.Count - 1;
    }
}
