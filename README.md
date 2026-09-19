# CS2 Bodycam Mod — Структура проекта и инструкция по сборке

```
CS2BodycamMod/
├── Mod/                         
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── dllmain.cpp          
│   │   ├── HookManager.h / .cpp 
│   │   └── SigScanner.h / .cpp  
│   └── thirdparty/
│       └── README.txt           
│
└── Injector/                    
    ├── CS2BodycamInjector.csproj
    ├── Program.cs
    ├── MainForm.cs
    ├── MainForm.Designer.cs
    ├── DllInjector.cs
    └── Resources/
        └── bodycam_mod.dll      
```

---

## Шаг 0 — Что нужно установить (один раз)

| Инструмент | Где взять | Зачем |
|---|---|---|
| **Visual Studio 2022** (Community — бесплатно) | visualstudio.microsoft.com | Компилятор MSVC для C++ |
| Компонент **"Разработка классических приложений C++"** | Установщик VS → Workloads | |
| Компонент **"Разработка классических приложений .NET"** | Установщик VS → Workloads | Для C# инжектора |
| **.NET 8 SDK** | dotnet.microsoft.com/download | `dotnet build` CLI |
| **CMake 3.22+** | cmake.org/download | Генератор проекта для C++ |
| **Git** | git-scm.com | Клонировать MinHook |

---

## Шаг 1 — Клонировать MinHook

Открой **Developer Command Prompt for VS 2022** (или обычный cmd/PowerShell в папке проекта):

```cmd
cd Mod
git clone https://github.com/TsudaKageyu/minhook thirdparty/minhook
```

---

## Шаг 2 — Собрать C++ DLL (bodycam_mod.dll)

```cmd
cd Mod

:: Генерируем Visual Studio проект (x64 ОБЯЗАТЕЛЬНО!)
cmake -B build -G "Visual Studio 17 2022" -A x64

:: Собираем в Release (меньше размер, нет debug overhead)
cmake --build build --config Release
```

После успешной сборки CMake **автоматически скопирует** `bodycam_mod.dll`
в `Injector/Resources/bodycam_mod.dll`.

Проверь:
```cmd
dir Injector\Resources\bodycam_mod.dll
```

---

## Шаг 3 — Собрать C# инжектор

```cmd
cd Injector

:: Восстановить зависимости и собрать
dotnet build -c Release

:: Собрать в ОДИН .exe файл (удобно для запуска)
dotnet publish -c Release -r win-x64 --self-contained false
```

Готовый exe будет в:
```
Injector/bin/Release/net8.0-windows/win-x64/publish/CS2BodycamInjector.exe
```

---

## Шаг 4 — Запуск

1. Запусти `CS2BodycamInjector.exe` **от имени администратора**
   (нужны права для `OpenProcess` + `CreateRemoteThread`)

2. Нажми **"Launch CS2 (-insecure)"** → игра запустится с флагом

3. Дождись загрузки в главное меню CS2

4. Нажми **"Inject Bodycam Mod"** → лог покажет результат

---

## Частые проблемы

| Проблема | Решение |
|---|---|
| `bodycam_mod.dll не вшит` | Пересобери C++ часть, проверь что файл появился в `Injector/Resources/` |
| `OpenProcess failed (5)` | Запусти инжектор **от Администратора** |
| `CreateRemoteThread failed` | Антивирус или Easy Anti-Cheat блокируют — добавь папку в исключения AV, убедись что CS2 запущен с `-insecure` |
| Хук не находит функцию (сигнатура устарела) | После обновления CS2 обнови сигнатуры в `HookManager.cpp` → ищи через IDA/x64dbg |
| Вылет CS2 при инъекции | Неправильный calling convention или смещение структуры — верифицируй офсеты через ReClass.NET |

---

## Архитектура модулей

```
dllmain.cpp
    └── ModThread()
            └── HookManager::Initialize()
                    ├── HookSetupView()     → Hooked_SetupView()
                    │                         [Phase 2: камера на кость viewmodel]
                    ├── HookCreateMove()    → Hooked_CreateMove()
                    │                         [Phase 2: deadzone свободного прицела]
                    ├── HookFireBullets()   → Hooked_FireBullets()
                    │                         [Phase 2: пуля из дула, не из глаз]
                    └── HookPaintTraverse() → Hooked_PaintTraverse()
                                              [Phase 3: лазерный луч + fisheye]
```

---

## Следующие шаги (Phase 2)

- [ ] Верифицировать офсеты структур `CViewSetup`, `CUserCmd` через **ReClass.NET**
- [ ] Реализовать `GetViewmodel()` и `GetBoneTransform()` для `rear_sight` аттачмента
- [ ] Заполнить `Hooked_SetupView` — переписать `origin`/`angles` из кости
- [ ] Заполнить `Hooked_CreateMove` — deadzone математика
- [ ] Заполнить `Hooked_FireBullets` — найти офсеты `FireBulletsInfo`
- [ ] Реализовать рейкаст лазера в `Hooked_PaintTraverse`
- [ ] Добавить fisheye через расширенный FOV + vignette overlay
