#pragma once

// ============================================================================
//  SigScanner.h  —  IDA-style pattern scanning across loaded DLL modules
//  C++20 | x64 only
// ============================================================================

#include <Windows.h>
#include <Psapi.h>
#include <cstdint>
#include <optional>
#include <string_view>

/// Сканирует загруженный в текущий процесс модуль по IDA-паттерну.
///
/// Формат паттерна (case-insensitive, пробелы разделяют байты):
///   "48 8B ? ? 89 45 ?? CC"
///   — "??" или "?" означают wildcard (любой байт)
///
/// Возвращает адрес первого совпадения или std::nullopt.
class SigScanner
{
public:
    [[nodiscard]]
    static std::optional<std::uintptr_t>
    FindPattern(std::string_view moduleName, std::string_view idaPattern) noexcept;

    /// Resolve relative call/jmp: читает 4-байтовый relative offset по addr+offset
    /// и возвращает абсолютный адрес назначения (addr + offset + 4 + disp32).
    [[nodiscard]]
    static std::uintptr_t ResolveRelative(std::uintptr_t addr, int offset = 1) noexcept;
};
