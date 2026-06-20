cmake_minimum_required(VERSION 3.22)
project(bodycam_mod LANGUAGES CXX)

# ============================================================================
#  Настройки компилятора
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Только x64 — cs2.exe 64-битный
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Собирай ТОЛЬКО x64! Переключи архитектуру в CMake.")
endif()

# ============================================================================
#  Исходники мода
# ============================================================================
add_library(bodycam_mod SHARED
    src/dllmain.cpp
    src/HookManager.cpp
    src/HookManager.h
    src/SigScanner.cpp
    src/SigScanner.h
)

# ============================================================================
#  MinHook (должен быть рядом в thirdparty/)
#  Скачай: https://github.com/TsudaKageyu/minhook
#  Распакуй так:   thirdparty/minhook/include/MinHook.h
#                  thirdparty/minhook/src/...
# ============================================================================
set(MINHOOK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/minhook")

# Собираем MinHook вместе с модом (статически)
set(MINHOOK_SOURCES
    ${MINHOOK_DIR}/src/buffer.c
    ${MINHOOK_DIR}/src/hook.c
    ${MINHOOK_DIR}/src/trampoline.c
    ${MINHOOK_DIR}/src/hde/hde64.c
)

target_sources(bodycam_mod PRIVATE ${MINHOOK_SOURCES})
target_include_directories(bodycam_mod PRIVATE
    src/
    ${MINHOOK_DIR}/include
)

# ============================================================================
#  Линковка системных библиотек
# ============================================================================
target_link_libraries(bodycam_mod PRIVATE
    Psapi      # GetModuleInformation (SigScanner)
    # DbgHelp  # раскомментируй если понадобится символьный lookup
)

# ============================================================================
#  Флаги компилятора (MSVC)
# ============================================================================
if(MSVC)
    target_compile_options(bodycam_mod PRIVATE
        /W4          # Предупреждения уровня 4
        /WX-         # Не превращать предупреждения в ошибки (пока)
        /MP          # Многопоточная сборка
        /permissive- # Строгий режим C++
        /Oi          # Intrinsics
        /Gy          # Function-level linking
    )

    # В Release — максимальные оптимизации
    target_compile_options(bodycam_mod PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob3 /GL>
    )
    target_link_options(bodycam_mod PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )

    # DEF-файл не нужен — у нас только DllMain
    # Убираем дефолтный mangling при экспорте (если понадобятся экспорты позже)
    # set_target_properties(bodycam_mod PROPERTIES LINK_FLAGS "/DEF:...")
endif()

# ============================================================================
#  После сборки — автокопирование DLL в папку инжектора
#  (чтобы не делать это руками каждый раз)
# ============================================================================
set(INJECTOR_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../Injector/Resources")
add_custom_command(TARGET bodycam_mod POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${INJECTOR_RESOURCES}"
    COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE:bodycam_mod>"
        "${INJECTOR_RESOURCES}/bodycam_mod.dll"
    COMMENT "→ Копирую DLL в Injector/Resources/bodycam_mod.dll"
)
