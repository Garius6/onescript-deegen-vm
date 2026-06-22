# Deegen Integration Plan

Реалистичный план Phase 2 — подключения Deegen baseline JIT к OneScript VM.

**Статус**: план, не реализация. Build Deegen может быть запущен в Docker x86_64 на arm64 хосте через qemu.

## После чтения Deegen-исходников

[Deegen](https://github.com/luajit-remake/luajit-remake) — это **VM-генератор**, привязанный к Lua семантике. Главные артефакты в репо:

- `annotated/bytecodes/` — Lua-опкоды (Add, Sub, Less, Return, TableGet и т.д.) описанные через Deegen DSL
- `runtime/` — Lua runtime: TValue (NaN-boxed), TableObject, HeapPtr, GC-инфраструктура, parser/lexer Lua
- `drt/`, `deegen/` — собственно meta-compiler, генерирует interpreter + baseline JIT из аннотаций
- `main.cpp` + `ljr-build` — entry-point `luajitr` который грузит и выполняет Lua-bytecode

**Ключевое наблюдение**: Deegen-LJR это **готовая Lua-VM**, а не "выбирай свой язык". Чтобы получить наш OneScript-VM через Deegen, два пути:

## Путь A — переводчик OneScript bytecode → Lua bytecode

Идея: компилируем `.osbin` в Lua-эквивалент, запускаем на LuaJIT Remake (= Deegen baseline JIT).

### Что нужно

1. **Переводчик** `osbin → .luac` (LuaJIT Remake принимает legacy JSON dump или Lua source).
   - Mapping опкодов:
     - OneScript `Add/Sub/Mul/Div/Mod` → Lua `Add/Sub/Mul/Div/Mod` (1:1, оба stack-based с регистровым backed-storage).
     - OneScript `PushLoc/LoadLoc` (через стек) → Lua `MOV` между регистрами (register-based в LuaJIT).
     - OneScript `CallFunc/CallProc/ArgNum` → Lua `CALL/CALLT` с переупаковкой аргументов.
     - OneScript `Jmp/JmpFalse` → Lua `JMP/IST/ISF + JMP`.
     - OneScript `Сообщить` → Lua `print` call.
   - Несовместимое: `MakeRawValue`, `Возврат` через jump на единый Return, бывшие inline-builtin опкоды (StrLen, Sin etc.) — для них writer нужно эмулировать или skip.
2. **Frame model adapter**: OneScript stack-based ↔ LuaJIT register-based.
   - Простой подход: на каждую OneScript-локалку выделяешь регистр в Lua frame. PushLoc/LoadLoc становятся MOV.
   - Стек значений эмулируется через выделение скрэтч-регистров.
3. **Тестовая оснастка**: `.os → osbc-export → .osbin → transpile → .luac → luajitr → result`.
4. **Бенчмарк**: fib(30), mandel, nbody через этот pipeline vs наш текущий stub.

### Плюсы

- **Никаких изменений в Deegen**. Используем как есть.
- **JIT работает из коробки** — все speedup-ы Deegen-LJR применяются автоматически.
- **Возможность переиспользовать Lua stdlib** для базовых вещей (print, math).
- **Срок реализации** реальный: 2-4 недели на спайк, ещё 2-3 на покрытие.

### Минусы

- **Косвенный путь**: OneScript семантика урезается до Lua-семантики. Не все опкоды переводятся 1:1.
- **Не "родной" OneScript JIT**: это "fast Lua который притворяется OneScript". Для пича это нюанс — апстрим-разработчики могут спросить "а что с настоящей семантикой?".
- **stdlib OneScript не работает** — нет Структуры, Массива и т.д. без отдельной реализации.
- **Error messages, stack traces** — на языке Lua, не OneScript.

## Путь B — собственный VM через Deegen meta-compiler

Идея: реализовать OneScript как полноценный язык в Deegen-стиле — наши опкоды, наш value model, наш runtime.

### Что нужно

1. **Форкнуть Deegen** или сделать отдельный submodule поверх Deegen libraries.
2. **Заменить Lua runtime** в `runtime/` на OneScript runtime:
   - Свой `TValue` (можно переиспользовать NaN-boxing layout): tDouble, tBool, tNil, tString, tHeapObj.
   - Свой `init_global_object` (instead of init Lua globals).
   - Убрать Lua parser/lexer — нам не нужны, мы загружаем готовый bytecode.
   - Свой loader `.osbin` (С++).
3. **Переписать `annotated/bytecodes/`** под OneScript-опкоды:
   - 25+ опкодов из stub (Add, Sub, Mul, ..., CallFunc, CallProc, Return, etc.) аннотируем через Deegen DSL.
   - ArgNum, MakeRawValue, LineNum — простейшие, no-ops или вспомогательные.
   - CallFunc с SDO method-bias logic — отдельный challenge.
4. **Перепилить main.cpp / `luajitr` entrypoint** под наш runtime.
5. **Сборка** — modify CMakeLists.txt либо отдельная сборка с подменёнными `runtime/`.

### Плюсы

- **Настоящая нативная OneScript-VM** с JIT.
- **Полная семантика** OneScript: Decimal-arithmetic (если нужно), specific error messages, exact stack traces.
- **Расширяемость** — добавление новых опкодов делается в одном файле.
- **Production-grade путь** — то что предлагается апстрим-разработчикам как новый backend.

### Минусы

- **Огромный объём работы**: 2-3 месяца full-time. Forking research-grade проекта = постоянная боль с rebases на upstream.
- **Глубокое погружение в Deegen internals**: TValue layout, RegAllocHint, DfgVariant, GHC CC nuances. Документация тонкая.
- **Может потребоваться апстрим-фикс** в Deegen для нетипичных опкодов (SDO bias, sigle Return at end, etc.).

## Рекомендация

**Для пича сообществу** — Путь A (transpiler). Меньшая инвестиция (3-6 недель), достаточная для демонстрации потенциала. Цифры будут реальные.

**Для production replacement** — Путь B. Только если апстрим OneScript-команда заинтересована и готова поддержать.

## Конкретные следующие шаги

### Старт пути A

1. Дописать билд Deegen в Docker — `python3 ljr-build make release` (запущено в фоне сейчас).
2. Прогнать `bash run_bench.sh` из Deegen на стоковом fib.lua. Зафиксировать baseline.
3. Написать минимальный `osbin → lua source` транспилятор на Python. Цель: fib.osbin → fib.lua → `print(Fib(10))`.
4. Скомпилировать через `luajitr` (использует Deegen JIT). Сверить результат с upstream OneScript.
5. Замерить fib(30), mandel, nbody, сравнить с upstream + текущий stub.
6. Если speedup впечатляет — расширять покрытие опкодов и стабильность.

### Старт пути B

1. Дописать билд Deegen (как и для A).
2. Прочитать paper "Deegen: A JIT-Capable VM Generator for Dynamic Languages" целиком.
3. Спайк: один опкод (Add) через Deegen DSL с нашими типами. **Это уже сложно**.
4. Расширить до 5 опкодов, потом 25. ~2-3 месяца.

## Что осталось понять (research items)

- Может ли `luajitr` принимать сторонний bytecode формат, или строго LuaJIT bytecode?
- Есть ли в Deegen mechanism для "guest language" hooks, или Lua-only?
- `Decimal` vs `double` — что делать с этой расхождением? OneScript использует Decimal для семантической точности.
- Можно ли в `luajitr` плавно подменять `print()` на наш `Сообщить()`?

## Запасной вариант — без Deegen

Если Deegen окажется тупиком, есть промежуточные точки:

- **Computed-goto interpreter** — 1-3 дня, +1.5-3× к текущему stub.
- **Copy-and-patch вручную через DynASM** — 2-3 недели, ожидаемо 3-10× к stub. Это "Deegen lite".
- **LLVM-based JIT через [Cranelift](https://github.com/bytecodealliance/wasmtime/tree/main/cranelift)** — 4-6 недель, 10-30× speedup, но огромная зависимость в рантайме.

Все три — fallback если Deegen не подойдёт по архитектурным причинам.
