// ============================================================================
//  dllmain.cpp  —  Точка входа DLL-мода
//  C++20 | x64 | CS2 / Source 2
// ============================================================================

#include "HookManager.h"

#include <Windows.h>
#include <thread>
#include <chrono>

// ============================================================================
//  Поток мода — ждёт загрузки всех нужных DLL и инициализирует хуки
// ============================================================================
static void ModThread(HMODULE hSelf) noexcept
{
    using namespace std::chrono_literals;

    // Ждём пока client.dll и engine2.dll загрузятся в память
    // (они грузятся после DLL_PROCESS_ATTACH, не сразу)
    while (!GetModuleHandleA("client.dll") ||
           !GetModuleHandleA("engine2.dll"))
    {
        std::this_thread::sleep_for(200ms);
    }

    // Небольшая пауза — даём движку завершить инициализацию интерфейсов
    std::this_thread::sleep_for(1000ms);

    // Инициализируем все хуки
    HookManager::Get().Initialize();

    // -------------------------------------------------------------------------
    //  Пока хуки работают — просто ждём.
    //  В Phase 2 здесь можно сделать render-loop или hotkey unload (End/Delete).
    // -------------------------------------------------------------------------
    //  Пример unload по нажатию клавиши END:
    //
    //  while (!(GetAsyncKeyState(VK_END) & 0x8000))
    //      std::this_thread::sleep_for(100ms);
    //  HookManager::Get().Shutdown();
    //  FreeLibraryAndExitThread(hSelf, 0);
    // -------------------------------------------------------------------------

    (void)hSelf;
}

// ============================================================================
//  DllMain
// ============================================================================
BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD   dwReason,
                      LPVOID  /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Не создаём отдельный поток на каждый новый поток хоста
        DisableThreadLibraryCalls(hModule);

        // Запускаем инициализацию в отдельном потоке:
        // DllMain вызывается под Loader Lock — блокировать здесь нельзя.
        std::thread(ModThread, hModule).detach();
        break;

    case DLL_PROCESS_DETACH:
        // Вызывается при FreeLibrary или завершении процесса
        HookManager::Get().Shutdown();
        break;
    }

    return TRUE;
}
