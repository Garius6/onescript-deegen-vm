# Deegen Integration Plan

Реалистичный план Phase 2 — подключения Deegen baseline JIT к OneScript VM.

**Статус**: план, не реализация. Build Deegen может быть запущен в Docker x86_64 на arm64 хосте через qemu.

## После чтения Deegen-исходников

[Deegen](https://github.com/luajit-remake/luajit-remake) — это **VM-генератор**, привязанный к Lua семантике. Главные артефакты в репо:

- `annotated/bytecodes/` — Lua-опкоды (Add, Sub, Less, Return, TableGet и т.д.) описанные через Deegen DSL
- `runtime/` — Lua runtime: TValue (NaN-boxed), TableObject, HeapPtr, GC-инфраструктура, parser/lexer Lua
- `drt/`, `deegen/` — собственно meta-compiler, генерирует interpreter + baseline JIT из аннотаций
- `main.cpp` + `ljr-build` — entry-point `luajitr` который грузит и выполняет Lua-bytecode

**Уточнение после повторного прохода**: ядро Deegen в `deegen/` и `drt/` **язык-агностично**. NaN-boxed `TValue` поддерживает универсальные типы (`tNil`, `tBool`, `tDouble`, `tInt32`, `tHeapEntity`) — этого хватает для PoC OneScript без переделки. Lua-специфика сосредоточена в:

- `annotated/bytecodes/` — Lua-опкоды (пишем свои)
- `runtime/` — Lua runtime: TableObject, Lua-строки, GC, parser/lexer Lua (заменяем минимальным своим)
- `main.cpp` — `luajitr` entry-point (пишем свой)
- Lua-специфичные subtypes в `tvalue.h` (`tTable`, `tString`, `tFunction`) — нам не нужны для PoC

Это означает что Путь B **намного дешевле**, чем казалось на первом прохождении. Не "форк всего", а "Deegen как библиотека/фреймворк". Сроки в таблице ниже исправлены.

Чтобы получить наш OneScript-VM через Deegen, два пути:

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

Идея: использовать `deegen/` (мета-компилятор) и `drt/` (TValue + heap helpers) как библиотеку, написать свой набор аннотированных опкодов + минимальный runtime для OneScript.

### Что нужно

1. **Подключить Deegen как зависимость**. Не форк — наша `vm/` директория подтягивает `third_party/deegen/deegen/` и `third_party/deegen/drt/` через CMake.
2. **Использовать готовый `TValue`** из `drt/tvalue.h`. Нам уже доступно:
   - `tNil` — для `Неопределено` ✓
   - `tBool` — для `Булево` ✓
   - `tDouble` — для `Число` ✓
   - `tHeapEntity` — для будущих heap-объектов (`Строка`, `Массив`, `Структура`)
3. **Написать `vm/annotated/bytecodes/onescript.cpp`** под наши опкоды:
   - 25+ опкодов из stub аннотируем через Deegen DSL: `DEEGEN_DEFINE_BYTECODE(Add) { ... }`.
   - Типовые гарды через `Op("lhs").HasType<tDouble>()` — даёт JIT speculations.
   - ArgNum, MakeRawValue, LineNum — простейшие, no-ops или вспомогательные.
   - CallFunc с SDO method-bias logic — главный нетривиальный опкод.
4. **Минимальный runtime** в `vm/drt-ext/`:
   - Frame layout: совместимый с Deegen ExecutionFrame (или адаптер).
   - `Сообщить` как built-in lib function через `api_define_lib_function.h`.
   - **GC не нужен** на PoC (только value-типы, без heap).
5. **Свой `main.cpp`** — загружает `.osbin`, конвертит в Deegen `CodeBlock`, запускает.
6. **Сборка** через тот же Docker image (Clang с GHC CC), но отдельный CMake target.

### Плюсы

- **Настоящая нативная OneScript-VM** с JIT.
- **Полная семантика** OneScript: точные error messages, exact stack traces.
- **Расширяемость** — добавление новых опкодов делается в одном `.cpp` файле.
- **Production-grade путь** — то что можно предлагать апстрим-разработчикам как новый backend.

### Минусы

- **Объём работы средний**: 3-6 недель на стартовый PoC (5 опкодов + fib), 2-3 месяца на полную замену stub-а. Не "форк всего", а использование Deegen как библиотеки.
- **Глубокое погружение в Deegen DSL**: TValue layout, RegAllocHint, DfgVariant, GHC CC nuances. Документация тонкая, основной источник — paper + код LJR.
- **Зависимость от research-grade проекта**: Deegen в активной разработке, breaking changes возможны. Желательно зафиксировать на конкретном commit.

## Рекомендация

Обе оценки пересмотрены вниз после повторного прохода:

- **Путь A** (transpiler): 2-3 недели на работающий fib + бенчмарки. Косвенно, но **очень быстро**.
- **Путь B** (Deegen-DSL VM): 3-6 недель на работающий spike, 2-3 месяца на полное покрытие. **Прямо и production-friendly**.

Для **пича сообществу** разумно: начать с Пути A, получить первые цифры за 2 недели. Если speedup впечатляет и есть интерес — переключиться на Путь B как long-term направление.

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
