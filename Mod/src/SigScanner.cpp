// ============================================================================
//  SigScanner.cpp
// ============================================================================

#include "SigScanner.h"

#include <cctype>
#include <charconv>
#include <vector>
#include <cstring>

#pragma comment(lib, "Psapi.lib")

// ---------------------------------------------------------------------------
// Internal: parse one IDA-pattern token into (byte, isMask)
// ---------------------------------------------------------------------------
struct PatternByte
{
    uint8_t  value;
    bool     wildcard;  // если true — value игнорируется при сравнении
};

static std::vector<PatternByte> ParsePattern(std::string_view pattern) noexcept
{
    std::vector<PatternByte> result;
    result.reserve(32);

    size_t i = 0;
    while (i < pattern.size())
    {
        // Пропускаем пробелы
        while (i < pattern.size() && pattern[i] == ' ') ++i;
        if (i >= pattern.size()) break;

        if (pattern[i] == '?')
        {
            result.push_back({ 0x00, true });
            // Пропускаем все подряд идущие '?' (обрабатываем и "?" и "??")
            while (i < pattern.size() && pattern[i] == '?') ++i;
        }
        else
        {
            uint8_t byte = 0;
            auto res = std::from_chars(pattern.data() + i,
                                       pattern.data() + std::min(i + 2, pattern.size()),
                                       byte, 16);
            result.push_back({ byte, false });
            i += static_cast<size_t>(res.ptr - (pattern.data() + i));
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
std::optional<std::uintptr_t>
SigScanner::FindPattern(std::string_view moduleName,
                         std::string_view idaPattern) noexcept
{
    HMODULE hModule = GetModuleHandleA(moduleName.data());
    if (!hModule) return std::nullopt;

    MODULEINFO modInfo{};
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
        return std::nullopt;

    const auto* base = reinterpret_cast<const uint8_t*>(modInfo.lpBaseOfDll);
    const size_t     size = modInfo.SizeOfImage;

    auto pattern = ParsePattern(idaPattern);
    if (pattern.empty()) return std::nullopt;

    const size_t patLen = pattern.size();

    for (size_t offset = 0; offset + patLen <= size; ++offset)
    {
        bool found = true;
        for (size_t j = 0; j < patLen; ++j)
        {
            if (!pattern[j].wildcard && base[offset + j] != pattern[j].value)
            {
                found = false;
                break;
            }
        }
        if (found)
            return reinterpret_cast<std::uintptr_t>(base + offset);
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
std::uintptr_t SigScanner::ResolveRelative(std::uintptr_t addr, int offset) noexcept
{
    //  E8 <disp32>   или   48 8D 05 <disp32>
    //  Абсолютный адрес = addr + offset + 4 + *reinterpret_cast<int32_t*>(addr+offset)
    int32_t disp = 0;
    std::memcpy(&disp, reinterpret_cast<const void*>(addr + offset), sizeof(disp));
    return addr + static_cast<std::uintptr_t>(offset) + 4u
           + static_cast<std::uintptr_t>(static_cast<std::ptrdiff_t>(disp));
}
