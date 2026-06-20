#pragma once

// ============================================================================
//  HookManager.h  —  MinHook wrapper + хуки всех модулей мода
//  C++20 | x64 | Source 2 / CS2
// ============================================================================

#include <MinHook.h>
#include <cstdint>
#include <source_location>
#include <vector>
#include <string_view>

// ============================================================================
//  Частичные определения структур Source 2 / CS2
//  (офсеты нужно верифицировать через ReClass.NET / Cheat Engine при каждом
//   обновлении игры — помечены комментарием OFFSET_VERIFY)
// ============================================================================
namespace S2
{

// Вектор трёх float'ов — совместим с Vector / QAngle движка
struct Vec3 { float x, y, z; };

// Частичный layout CViewSetup (client.dll)
// Точные офсеты — через ReClass на живом процессе
struct CViewSetup
{
    /* 0x00 */ int   x, y;             // экранные координаты viewport
    /* 0x08 */ int   width, height;
    /* 0x10 */ float fov;
    /* 0x14 */ float fovViewmodel;
    /* 0x18 */ float zNear, zFar;
    /* 0x20 */ uint8_t _pad20[0x10];   // OFFSET_VERIFY
    /* 0x30 */ Vec3    origin;          // позиция камеры
    /* 0x3C */ Vec3    angles;          // углы камеры (pitch/yaw/roll)
    // ... остальные поля не нужны на первом этапе
};

// Минимальная обёртка CUserCmd (данные ввода кадра)
// Реальный layout зависит от версии CS2
struct CUserCmd
{
    uint8_t _pad0[0x10];              // OFFSET_VERIFY
    Vec3    viewAngles;               // желаемые углы взгляда
    float   forwardMove;
    float   sideMove;
    float   upMove;
    uint64_t buttons;
    // ...
};

} // namespace S2

// ============================================================================
//  Singleton HookManager
// ============================================================================
class HookManager
{
public:
    static HookManager& Get() noexcept
    {
        static HookManager s_instance;
        return s_instance;
    }

    // Non-copyable / non-movable
    HookManager(const HookManager&)            = delete;
    HookManager& operator=(const HookManager&) = delete;

    // Точки входа — вызываются из ModThread
    bool Initialize();
    void Shutdown();

    // -------------------------------------------------------------------------
    //  Шаблонный хелпер: создаёт и сразу включает один хук.
    //  Fn — тип указателя на функцию (e.g. void(__fastcall*)(void*,...))
    //
    //  target     — адрес перехватываемой функции
    //  hookFn     — наш хук
    //  originalFn — [out] трамплин к оригинальной функции
    //
    //  Параметр loc нужен только для потенциального логирования/отладки.
    // -------------------------------------------------------------------------
    template<typename Fn>
    bool InstallHook(std::uintptr_t                 target,
                     Fn                             hookFn,
                     Fn*                            originalFn,
                     std::source_location           loc = std::source_location::current())
    {
        MH_STATUS status = MH_CreateHook(
            reinterpret_cast<void*>(target),
            reinterpret_cast<void*>(hookFn),
            reinterpret_cast<void**>(originalFn)
        );
        if (status != MH_OK) return false;

        status = MH_EnableHook(reinterpret_cast<void*>(target));
        if (status == MH_OK)
            m_hookedTargets.push_back(reinterpret_cast<void*>(target));
        return status == MH_OK;
    }

private:
    HookManager() = default;

    // ── Установщики хуков ────────────────────────────────────────────────────
    bool HookSetupView();       // Модуль 1: камера на viewmodel
    bool HookCreateMove();      // Модуль 2: deadzone свободного прицела
    bool HookFireBullets();     // Модуль 3: стрельба из ствола

    // ── Runtime draw-hook (лазер + fisheye) ──────────────────────────────────
    bool HookPaintTraverse();   // Модуль 4+5: рисование лазера, HUD

    std::vector<void*> m_hookedTargets;
    bool               m_initialized = false;
};
