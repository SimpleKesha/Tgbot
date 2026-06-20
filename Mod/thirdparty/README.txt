MinHook нужен для сборки мода.

Скачай один раз:
  git clone https://github.com/TsudaKageyu/minhook thirdparty/minhook

После клона папка должна выглядеть так:
  thirdparty/
  └── minhook/
      ├── include/
      │   └── MinHook.h
      └── src/
          ├── buffer.c
          ├── hook.c
          ├── trampoline.c
          └── hde/
              ├── hde64.c
              └── hde64.h

Ничего компилировать отдельно не нужно — CMakeLists.txt включает
исходники MinHook прямо в сборку мода.
