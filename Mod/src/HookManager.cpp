// ============================================================================
//  HookManager.cpp
//  Все хуки вынесены в статические функции в анонимном namespace,
//  чтобы не загрязнять глобальное пространство имён.
// ============================================================================

#include "HookManager.h"
#include "SigScanner.h"

#include <Windows.h>
#include <cmath>

// ============================================================================
//  IDA-сигнатуры CS2 (client.dll, build ~1.41.x, июнь 2025)
//  При обновлении игры находить заново через IDA/Ghidra или ce-sig плагин.
//  Используй Ctrl+B в Cheat Engine или "Find pattern" в IDA после обновления.
// ============================================================================
namespace Sigs
{
    // CCSPlayerController::SetupView или CViewRender::SetupView
    constexpr std::string_view SetupView =
        "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 60";

    // CCSGOInput::CreateMove (обработка ввода)
    constexpr std::string_view CreateMove =
        "40 55 53 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ?";

    // CWeaponCSBase::FireBullets или C_CSWeaponBase::FireBullets
    constexpr std::string_view FireBullets =
        "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57";

    // PaintTraverse (VGUI / HUD frame render)
    constexpr std::string_view PaintTraverse =
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 0F B6 82";
}

// ============================================================================
//  Typedefs оригинальных функций
// ============================================================================
using SetupView_t     = void(__fastcall*)(void* thisPtr, S2::CViewSetup* pSetup, void* pHead);
using CreateMove_t    = bool(__fastcall*)(void* thisPtr, float inputSampleTime, S2::CUserCmd* pCmd);
using FireBullets_t   = void(__fastcall*)(void* thisPtr, void* pFireBulletsInfo);
using PaintTraverse_t = void(__fastcall*)(void* thisPtr, unsigned int vguiPanel, bool forceRepaint, bool allowForce);

// Трамплины к оригинальным функциям (заполняет MinHook)
static SetupView_t     g_oSetupView     = nullptr;
static CreateMove_t    g_oCreateMove    = nullptr;
static FireBullets_t   g_oFireBullets   = nullptr;
static PaintTraverse_t g_oPaintTraverse = nullptr;

// ============================================================================
//  ──────────────────────────────────────────────────────────────────────────
//  МОДУЛЬ 1: Хук SetupView — камера прикреплена к viewmodel
//  ──────────────────────────────────────────────────────────────────────────
// ============================================================================
namespace /*anonymous*/
{

void __fastcall Hooked_SetupView(void* thisPtr, S2::CViewSetup* pSetup, void* pHead)
{
    // Сначала даём оригинальной функции заполнить pSetup стандартными данными
    g_oSetupView(thisPtr, pSetup, pHead);

    // ── TODO Phase 2 ─────────────────────────────────────────────────────────
    //
    // 1. Получить указатель на локального игрока:
    //      auto* pLocal = *reinterpret_cast<void**>(
    //          SigScanner::ResolveRelative(Sigs::LocalPlayer, 3));
    //
    // 2. Получить C_BaseViewModel из локального игрока:
    //      auto* pVM = GetViewmodel(pLocal);   // через offset или интерфейс
    //
    // 3. Получить матрицу кости "rear_sight" (или аттачмент muzzle_flash):
    //      matrix3x4_t boneMat;
    //      pVM->GetBoneTransform(BONE_REAR_SIGHT, boneMat);
    //
    // 4. Переписать позицию и углы камеры:
    //      pSetup->origin = { boneMat[0][3], boneMat[1][3], boneMat[2][3] };
    //      pSetup->angles = MatrixToAngles(boneMat);
    //
    // ─────────────────────────────────────────────────────────────────────────
    //  Сейчас это no-op: хук установлен, оригинал вызван, данные не изменены.
    //  Закомментируй строку g_oSetupView выше, когда начнёшь фазу 2.
    // ─────────────────────────────────────────────────────────────────────────
}

// ============================================================================
//  МОДУЛЬ 2: Хук CreateMove — свободное прицеливание с deadzone
// ============================================================================

// Состояние deadzone (хранится между кадрами)
static S2::Vec3 s_vmOffset = {};   // накопленное смещение угла оружия
constexpr float kDeadzoneRadius = 8.0f;  // градусы — подбирается эмпирически

void __fastcall Hooked_CreateMove(void* thisPtr,
                                   float inputSampleTime,
                                   S2::CUserCmd* pCmd)
{
    // ── TODO Phase 2 ─────────────────────────────────────────────────────────
    //
    // 1. Читаем delta мыши из pCmd->viewAngles по сравнению с предыдущим кадром.
    //
    // 2. Добавляем дельту к s_vmOffset (смещение ствола на экране):
    //      s_vmOffset.x += delta.x;
    //      s_vmOffset.y += delta.y;
    //
    // 3. Clamp по deadzone:
    //      float dist = sqrtf(s_vmOffset.x*s_vmOffset.x + s_vmOffset.y*s_vmOffset.y);
    //      if (dist > kDeadzoneRadius)
    //      {
    //          float overflow = dist - kDeadzoneRadius;
    //          // Вектор нормали смещения
    //          float nx = s_vmOffset.x / dist;
    //          float ny = s_vmOffset.y / dist;
    //          // Overflow передаётся в реальный поворот тела
    //          pCmd->viewAngles.x += nx * overflow;
    //          pCmd->viewAngles.y += ny * overflow;
    //          // Клампим буфер обратно до радиуса
    //          s_vmOffset.x = nx * kDeadzoneRadius;
    //          s_vmOffset.y = ny * kDeadzoneRadius;
    //      }
    //      else
    //      {
    //          // Внутри зоны — тело не двигается, обнуляем delta в pCmd
    //          pCmd->viewAngles = s_prevAngles;  // оставляем прошлый угол
    //      }
    //
    // 4. Применяем s_vmOffset как кватернион/Euler к viewmodel через
    //    SetAbsAngles или аналог.
    //
    // ─────────────────────────────────────────────────────────────────────────

    // Пока просто передаём управление оригиналу
    g_oCreateMove(thisPtr, inputSampleTime, pCmd);
}

// ============================================================================
//  МОДУЛЬ 3: Хук FireBullets — пуля из ствола, не из глаз
// ============================================================================

void __fastcall Hooked_FireBullets(void* thisPtr, void* pFireBulletsInfo)
{
    // ── TODO Phase 2 ─────────────────────────────────────────────────────────
    //
    // FireBulletsInfo содержит поля:
    //   Vec3  m_vecSrc;       // начальная точка трассировки
    //   Vec3  m_vecDirShooting; // направление выстрела
    //   ...
    //
    // 1. Получить viewmodel аттачмент "muzzle_flash" в мировых координатах:
    //      Vec3 muzzleWorldPos = GetAttachmentWorldPos(pVM, "muzzle_flash");
    //      Vec3 barrelDir      = GetAttachmentForward(pVM, "muzzle_flash");
    //
    // 2. Переписать поля напрямую:
    //      reinterpret_cast<Vec3*>(
    //          reinterpret_cast<uintptr_t>(pFireBulletsInfo) + 0x??)->x = muzzleWorldPos.x;
    //      // и т.д.
    //
    // ─────────────────────────────────────────────────────────────────────────

    g_oFireBullets(thisPtr, pFireBulletsInfo);
}

// ============================================================================
//  МОДУЛЬ 4+5: PaintTraverse — лазерный луч + fisheye post-process
// ============================================================================

void __fastcall Hooked_PaintTraverse(void*        thisPtr,
                                      unsigned int vguiPanel,
                                      bool         forceRepaint,
                                      bool         allowForce)
{
    g_oPaintTraverse(thisPtr, vguiPanel, forceRepaint, allowForce);

    // ── TODO Phase 3 ─────────────────────────────────────────────────────────
    //
    // ЛАЗЕРНЫЙ ЛУЧ (Module 4):
    //   Каждый кадр (только для "HudPanel" или корневой панели):
    //   1. TraceLine из muzzle_flash forward → записать endPos
    //   2. Проецировать startPos и endPos в экранные координаты
    //      через g_pEngineClient->WorldToScreenMatrix()
    //   3. Нарисовать линию через vgui::surface()->DrawLine()
    //      или beam_effects (IBeam)
    //
    // FISHEYE / BODYCAM (Module 5):
    //   Через CPostProcessController или EnvToneMap:
    //   1. Поднять fov до ~120 (рыбий глаз через широкий FOV в CViewSetup)
    //   2. Применить виньетирование краёв (screen-space overlay,
    //      тёмный alpha-gradient rect от краёв к центру через vgui::surface)
    //   3. Опционально: CColorCorrection для зернистости/насыщенности
    //
    // ─────────────────────────────────────────────────────────────────────────
}

} // anonymous namespace

// ============================================================================
//  HookManager::Initialize / Shutdown
// ============================================================================

bool HookManager::Initialize()
{
    if (m_initialized) return true;

    if (MH_Initialize() != MH_OK)
        return false;

    // Устанавливаем все хуки. Если хоть один не встал — не паникуем,
    // логируем и продолжаем (частичная функциональность лучше краша).
    bool sv  = HookSetupView();
    bool cm  = HookCreateMove();
    bool fb  = HookFireBullets();
    bool pt  = HookPaintTraverse();

    // Для отладки: можно подключить OutputDebugStringA здесь
    (void)sv; (void)cm; (void)fb; (void)pt;

    m_initialized = true;
    return true;
}

void HookManager::Shutdown()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    m_hookedTargets.clear();
    m_initialized = false;
}

// ============================================================================
//  Установщики конкретных хуков
// ============================================================================

bool HookManager::HookSetupView()
{
    auto addr = SigScanner::FindPattern("client.dll", Sigs::SetupView);
    if (!addr) return false;
    return InstallHook(*addr,
        reinterpret_cast<SetupView_t>(Hooked_SetupView),
        &g_oSetupView);
}

bool HookManager::HookCreateMove()
{
    auto addr = SigScanner::FindPattern("client.dll", Sigs::CreateMove);
    if (!addr) return false;
    return InstallHook(*addr,
        reinterpret_cast<CreateMove_t>(Hooked_CreateMove),
        &g_oCreateMove);
}

bool HookManager::HookFireBullets()
{
    auto addr = SigScanner::FindPattern("client.dll", Sigs::FireBullets);
    if (!addr) return false;
    return InstallHook(*addr,
        reinterpret_cast<FireBullets_t>(Hooked_FireBullets),
        &g_oFireBullets);
}

bool HookManager::HookPaintTraverse()
{
    auto addr = SigScanner::FindPattern("client.dll", Sigs::PaintTraverse);
    if (!addr) return false;
    return InstallHook(*addr,
        reinterpret_cast<PaintTraverse_t>(Hooked_PaintTraverse),
        &g_oPaintTraverse);
}
