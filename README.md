# onescript-vm

> ## ⚠️ ОСТОРОЖНО! НЕЙРОСЛОП
>
> Весь код, документация и замеры в этом репозитории сгенерированы с помощью LLM (Claude Opus 4.7) в режиме pair-programming. Человек задавал направление, проверял результаты и принимал решения; LLM писал код и текст.
>
> Это означает:
> - Код **не прошёл** code review человеком построчно.
> - Возможны subtle баги, неаккуратные edge cases, странные архитектурные решения.
> - Документация может содержать преувеличения, неточности или галлюцинации (несмотря на верификацию замеров).
> - Используй на свой страх и риск. Перед любым нетривиальным использованием — читай код и проверяй сам.
>
> Цель PoC — продемонстрировать **идею** (Deegen + OneScript bytecode) и собрать обратную связь сообщества, а не отгрузить production-ready решение.

> ## 🚧 СТАТУС: Phase 1 (stub). Deegen НЕ интегрирован
>
> Несмотря на название репо, **Deegen JIT в этом репозитории ещё не использован**. Сейчас в VM работает простой `switch`-based интерпретатор на C++ (`vm/src/interp.cpp`). Цифры из секции TL;DR — реальные замеры этого stub-а.
>
> **Что про Deegen уже сделано:**
> - Клон Deegen лежит в `third_party/deegen/` (не собирался).
> - Архитектура pipeline спроектирована так, чтобы stub можно было заменить на Deegen-generated VM без переделки остального.
>
> **Что про Deegen НЕ сделано:**
> - Сборка Deegen (требует Linux x86-64 + Docker, отдельная задача).
> - Аннотация опкодов в Deegen-DSL.
> - Замена `switch`-loop на copy-and-patch stencils.
> - Все цифры "ожидаемо 15-50×" про Phase 2 — **спекуляция**, не результат. См. `docs/expectations.md` если/когда появится с обоснованием.

---

**Proof-of-concept альтернативной высокопроизводительной VM для [OneScript](https://github.com/EvilBeaver/OneScript).**

Берём готовый bytecode апстрим-компилятора OneScript и исполняем его в собственной C++ VM. Цель — показать сообществу что текущий CLR-based интерпретатор можно заменить нативной VM с baseline JIT и получить кратный speedup без потери совместимости с существующим компилятором и языком.

---

## TL;DR — главный результат

| Бенчмарк | Upstream `oscript` (CLR) | Этот PoC (stub C++ VM, **без JIT**) | Speedup |
|----------|-------------------------:|------------------------------------:|--------:|
| `fib(30)` recursive               | 0.843 s | **0.264 s** | **3.19×** |
| `mandel(120×80, 200)` float math  | 0.537 s | **0.061 s** | **8.80×** |
| `nbody(50000 шагов)` 3-body sim   | 2.006 s | **0.287 s** | **6.99×** |
| Холодный старт                    | ~600 ms (.NET + JIT warmup) | ~10 ms | ~60× |

Геометрическое среднее: **~6×** на compute-heavy задачах. **Без какого-либо JIT** — это просто `switch`-based интерпретатор на C++.

> **Прогноз (не результат)**: при подключении baseline JIT через [Deegen](https://github.com/luajit-remake/luajit-remake) (Haoran Xu, copy-and-patch compilation) **ожидаем** дополнительный 5-15× поверх. Итог vs upstream: ориентир 30-100×. **Эти числа — спекуляция**, основанная на paper-замерах Deegen-LJR (`+28% к LuaJIT interpreter` + baseline JIT speedup) и интерполяции от типичного gap CLR↔native. Реальные числа появятся когда Phase 2 будет сделан.

---

## Идея и архитектура

OneScript состоит из трёх компонентов: lexer/parser → compiler → stack VM. Парсер и компилятор работают отлично. **VM — узкое место**.

Стратегия PoC та же что у JVM/CLR: компилятор остаётся как есть, заменяется только runtime. Bytecode становится "межплатформенным" форматом.

```
.os (исходник)
    │
    ▼
┌─────────────────────────┐
│ OneScript               │  Апстрим, C#, не меняется.
│ CompilerFrontend (C#)   │  Делает синтаксический разбор, типизацию,
└─────────────────────────┘  кодогенерацию bytecode.
    │
    ▼  StackRuntimeModule (in-memory)
┌─────────────────────────┐
│ osbc-export             │  Наша C# bootstrap-утилита (~300 строк).
│ (C# bridge)             │  Сериализует модуль в простой бинарный формат.
└─────────────────────────┘
    │
    ▼  .osbin (наш формат, 355 байт для fib.os)
┌─────────────────────────┐
│ osvm — C++ VM           │  Phase 1: switch-based интерпретатор (уже работает).
│                         │  Phase 2: Deegen baseline JIT (Linux x86-64).
└─────────────────────────┘
    │
    ▼
результат
```

### Почему это работает

OneScript bytecode имеет фиксированную форму: 126 опкодов, каждый — `(opcode: u8, arg: i32)`. Структура модуля проста: pools констант/идентификаторов, методы, code-секция, refs-таблицы.

Текущий dispatch в апстриме (`ScriptEngine.Machine.MachineInstance.MainCommandLoop`):

```csharp
while (...)
{
    var command = _module.Code[_currentFrame.InstructionPointer];
    _commands[(int) command.Code](command.Argument);   // ← virtual delegate call
}
```

Это **массив делегатов**, каждый вызов = indirect call через слот. CLR не инлайнит, branch prediction не помогает. Наш `switch` с inline-арифметикой обходит без особых усилий.

### Что не меняем

- Парсер — упстрим, статья совместимости bit-for-bit.
- Семантику языка — `Если/Тогда/Цикл/Возврат` работают как у апстрима.
- Опкоды — наш VM понимает upstream-bytecode, никаких трансляций.

### Что меняем

- Только VM tier (последний слой pipeline-а).
- Формат сериализации (текущий `BinaryFormatter` deprecated в .NET 5+ — наш `.osbin` решает заодно эту проблему).

---

## Замеры

### Окружение

- Host: macOS, Apple Silicon (arm64).
- Toolchain: Apple clang 17.0, .NET 8.0.128.
- Upstream OneScript: ветка `develop` (см. `research/upstream`).

```bash
$ bash benchmarks/run_bench.sh benchmarks/fib30.os benchmarks/mandel.os benchmarks/nbody.os
```

### fib(30)

```
upstream:  user 0m0.843s
ours:      user 0m0.264s   → 3.19× speedup
```

Рекурсия + integer compare + frame setup. Stresses call dispatch.

### mandel(120×80, 200 iter)

```
upstream:  user 0m0.537s   → 1661 точек внутри множества
ours:      user 0m0.061s   → 1661 (совпадает)  → 8.80× speedup
```

Float multiply/add в горячем двойном цикле. Самый большой gap потому что upstream использует `decimal` для всех чисел, а наш PoC — `double`.

### nbody(50000 шагов)

```
upstream:  user 0m2.006s   → -0.165984 → -0.165985 (Energy)
ours:      user 0m0.287s   → -0.165984 → -0.165985 (совпадает)  → 6.99× speedup
```

3 хардкоженных тела (Sun + Jupiter + Saturn). Leapfrog integration. Глобальные `Перем`-переменные, ручной `Корень()` через Newton's method.

Ключевая метрика — **user CPU time** (чистый compute). Wall-clock real одинаков из-за overhead-а тестовой обвязки (запуск `dotnet`-runner-а внутри shell-скрипта).

Полные результаты + методология: `benchmarks/RESULTS.md`.

---

## Запуск

### Зависимости

- **.NET 8 SDK** — для `osbc-export` (бутстрап-утилита). На macOS: `brew install dotnet@8`.
- **C++ compiler** с поддержкой C++20 (clang 14+, gcc 12+).
- **make** или **cmake 3.20+** (CMakeLists.txt включён).
- **git** — клонирование апстрима для path-based ProjectReference в C#.

### Первый запуск

```bash
# 1. Клонируем апстрим OneScript для reference и компиляции
git clone https://github.com/EvilBeaver/OneScript research/upstream

# 2. Генерируем opcode mapping из upstream Core.cs
bash scripts/gen_opcodes.sh
bash scripts/gen_opcode_headers.sh

# 3. Собираем C++ VM (cross-platform)
make -C vm

# 4. Прогоняем unit-тесты — fib(10) на hand-rolled модуле
make -C vm test

# 5. End-to-end: компилируем real .os через apstream, запускаем в нашем VM
export DOTNET_ROOT=/opt/homebrew/opt/dotnet@8/libexec
bash tests/e2e/run_e2e.sh
# → result: 55
# → PASS

# 6. Бенчмарк
bash benchmarks/run_bench.sh benchmarks/fib30.os
```

### Утилиты

- `vm/build/osvm <file.osbin>` — запустить bytecode-модуль.
- `vm/build/osdis <file.osbin>` — дизассемблер. Незаменим для отладки.
- `dotnet run --project tools/osbc-export -- <input.os> <output.osbin>` — компиляция через апстрим + сериализация в наш формат.

---

## Структура репозитория

```
docs/
├── bytecode-format.md     Анализ внутреннего устройства upstream bytecode-а
└── osbin-format.md        Спецификация нашего сериализационного формата

tools/
├── opcodes.csv            126 опкодов с их значениями (генерится из Core.cs)
└── osbc-export/           C# bootstrap утилита
    ├── Compilation.cs     Создание HostedScriptEngine, вызов CompilerFrontend
    ├── OsbinWriter.cs     Сериализация StackRuntimeModule в .osbin
    ├── OpCodes.g.cs       Сгенерированный mapping OperationCode → u8
    └── SilentHost.cs      Заглушка IHostApplication

vm/
├── include/onescript/     Public headers (Module, Value, Loader, Interp)
├── src/
│   ├── loader.cpp         Парсер .osbin
│   ├── value.cpp          Value type (Undefined, Number, Boolean, String)
│   ├── interp.cpp         Stub switch-based интерпретатор
│   ├── disasm.cpp         Дизассемблер (osdis)
│   └── main.cpp           CLI entry-point (osvm)
├── tests/                 Unit-тесты (test_loader, test_interp, test_fib)
├── Makefile               Fallback для систем без cmake
└── CMakeLists.txt

tests/
├── e2e/
│   ├── fib.os             Эталонный .os скрипт
│   └── run_e2e.sh         End-to-end runner: .os → .osbin → VM → check
└── fixtures/              Сгенерированные .osbin для тестов

benchmarks/
├── fib30.os               Бенчмарк-скрипт
├── run_bench.sh           Запуск + сравнение с upstream
└── RESULTS.md             Замеры

third_party/
└── deegen/                Клон LuaJIT Remake / Deegen (для Phase 2)

research/
└── upstream/              Клон upstream OneScript для reference
                           (ProjectReference из osbc-export указывает сюда)

scripts/
├── gen_opcodes.sh         Регенерация opcodes.csv из Core.cs
└── gen_opcode_headers.sh  Регенерация C++/C# headers из CSV
```

---

## Реализованный объём

### ✅ Готово

| Компонент | Описание | LOC (примерно) |
|-----------|----------|----------------|
| `.osbin` формат | Спецификация, документ | — |
| Opcode mapping | Генератор из upstream `Core.cs`, источник истины для C# и C++ | 50 |
| C# osbc-export | Бутстрап-утилита: грузит upstream-компилятор, пишет наш формат | 350 |
| C++ loader | Парсер `.osbin` с валидацией | 150 |
| C++ Value type | Undefined / Number / Boolean / String | 60 |
| Stub interpreter | 20+ опкодов: арифметика, сравнения, ветвления, вызовы, ArgNum, MakeRawValue, рекурсия | 350 |
| Disassembler `osdis` | Дамп опкодов + pools, незаменим при отладке | 80 |
| Unit-тесты | test_loader, test_interp, test_fib | 200 |
| E2E pipeline | shell-скрипт `.os → .osbin → run → assert` | 40 |
| Benchmark harness | Сравнение с upstream `oscript` | 30 |

### Реализованные опкоды (25+ из 126)

```
Stack/vars: PushVar, LoadVar, PushConst, PushInt, PushBool, PushUndef,
            PushLoc, LoadLoc, AssignRef, PushTmp, PopTmp
Math:       Add, Sub, Mul, Div, Mod, Neg, Inc
Compare:    Equals, NotEqual, Less, Greater, LessOrEqual, GreaterOrEqual
Logic:      Not, And, Or
Flow:       Jmp, JmpFalse, Return
Calls:      CallFunc, CallProc, ArgNum, MakeRawValue, MakeBool
Misc:       Nop, LineNum
```

Этого достаточно для compute-heavy бенчмарков (fibonacci, Mandelbrot, n-body, tight loops).

### Реализованные типы

- `Число` — backed by `double` (без integer fast-path в PoC, но всё равно обходим upstream).
- `Булево`.
- `Строка` — для I/O через `Сообщить`.
- `Неопределено`.

### Builtin-функции

- `Сообщить()` — print to stdout (через эвристику на `SystemGlobalContext` ref).

---

## Сложности реализации

Самые неочевидные места — для тех кто захочет повторить или расширить.

### 1. SDO method index bias

Самое коварное. `MemberNumber` в `MethodRefs` это **не** прямой индекс в `mod.Methods`. Upstream использует:

```csharp
mod.Methods[sdo.GetMethodDescriptorIndex(methodRef.MemberNumber)]
                  // = memberNumber - METHOD_COUNT
```

`METHOD_COUNT` = число inherited методов на `ScriptDrivenObject`. Для голого скрипта = 1. В нашем PoC захардкожено как `kScriptMethodBias = 1`.

При этом для `ModuleBody` (entry method) bias **не применяется** — прямой индекс. Асимметрия в upstream, легко проглядеть.

### 2. ArgNum опкод

Не очевидно из имени. Это **опкод который пушит число аргументов на стек прямо перед `CallFunc`/`CallProc`**. Без него интерпретатор не знает сколько аргументов pop-ать. Понял из `MachineInstance.PopArguments()`.

### 3. MakeRawValue / MakeBool

Имена обманчивы. Похоже на конверсию. На самом деле — **маркеры** для boxing/unboxing в upstream-реализации. Для нашей unboxed VM = `nop`.

### 4. Body без `Return`

`$entry` (тело модуля) генерируется **без** финального `Return` опкода. Когда IP уходит за конец массива — нужно молча выйти из фрейма, а не кидать "ip out of range". Иначе VM падает при первом запуске реального скрипта.

### 5. BinaryFormatter в апстриме

Текущая сериализация модуля = .NET `BinaryFormatter`. Deprecated с .NET 5, проприетарный CLR-формат. Прямой загрузке из C++ не подлежит. Поэтому пишем свой `.osbin`.

Совет апстрим-разработчикам: формат `.osbin` (или подобный простой бинарный) хорошо бы добавить как второй вариант сериализации — он решает проблему deprecated `BinaryFormatter` и одновременно открывает дорогу альтернативным VM.

### 6. Доступ к internal-полям через рефлексию

`StackRuntimeModule.Identifiers`, `VariableRefs`, `MethodRefs` — `internal`. `MachineMethodInfo` — `internal class`. Чтобы добраться из стороннего C# проекта, в `OsbinWriter.cs` использован `BindingFlags.NonPublic | BindingFlags.Instance`. Не идеально, но работает.

---

## Roadmap

### Phase 1 — Cross-platform PoC ✅

- [x] Research bytecode upstream
- [x] Спека `.osbin`
- [x] C# osbc-export
- [x] C++ loader + stub interpreter
- [x] E2E pipeline
- [x] Benchmark vs upstream → **3.2× user CPU без JIT**

### Phase 2 — Deegen baseline JIT 🔜

- [ ] Linux x86-64 Docker сборка Deegen
- [ ] Аннотировать опкоды как Deegen DSL (C++ с GHC calling convention + `musttail`)
- [ ] Build-time генерация stencils через Clang
- [ ] Подменить switch-dispatch на copy-and-patch JIT
- [ ] Замеры: ожидаемо **15-50× total** vs upstream

Deegen требует Linux + Docker — на macOS работать не будет. Для самого пича можно показать Phase 1.

### Phase 3 — расширение покрытия

- [ ] Массив, Структура, Соответствие
- [ ] Iterator opcodes (`PushIterator`, `IteratorNext`, `StopIterator`)
- [ ] BeginTry/EndTry/RaiseException
- [ ] Полный set builtin-функций (StrLen, Sin, Format и т.д.)
- [ ] Object methods через `ResolveProp`/`ResolveMethodFunc`

### Что НЕ планируется

- **.NET FFI / COM**. Принципиальный отказ — это убивает идею standalone VM. Скрипты которые используют `Новый COMОбъект()` или подключаемые .NET DLL не работают.
- **Production drop-in replacement**. Цель PoC — пич, не замена. Если сообщество захочет двигаться дальше — отдельный проект.

---

## Вопросы и ответы

### Это форк OneScript?

Нет. Это **альтернативный backend** для существующего OneScript-компилятора. Парсер, лексер, типизация, semantic analysis — всё это берётся из upstream без изменений. Меняется только finальный исполнительный слой.

### Зачем без stdlib?

Чтобы доказать сам тезис (производительность стэк-VM можно сильно поднять). Stdlib — это сотни функций, каждая ~50 строк. Расширяется тривиально, но это работа на месяцы. Для пича достаточно compute-only бенчмарков.

### Почему C++ а не Rust/Zig/Odin?

Deegen написан и работает с C++ (через Clang + LLVM офлайн). Чтобы Deegen-stencils работали seamlessly, базовая VM должна быть на C++. В Phase 1 (без Deegen) можно было бы и Rust/Zig — но Phase 2 требует C++.

### Что с Windows?

Сейчас не тестировалось. CMake/Makefile сборка должна работать с MinGW/MSVC. Deegen-фаза для Windows потребует доп. работы (PE/COFF relocations).

### Можно ли использовать этот код в проде?

Нет. Это PoC. Нет stdlib, нет полного покрытия опкодов, нет тестов на edge cases, нет GC, нет полноценной обработки ошибок.

### Что если апстрим заметно изменит bytecode?

`scripts/gen_opcodes.sh` регенерирует CSV из `Core.cs`. Изменения в опкодах подхватятся автоматически. Изменения в семантике (новый bias, новый header модуля) потребуют точечных правок в `OsbinWriter.cs` и `interp.cpp`.

---

## Документация (для тех кто хочет копнуть глубже)

- [`docs/bytecode-format.md`](docs/bytecode-format.md) — полный разбор upstream bytecode-а: опкоды, структура модуля, сериализация, dispatch.
- [`docs/osbin-format.md`](docs/osbin-format.md) — спецификация нашего бинарного формата.
- [`benchmarks/RESULTS.md`](benchmarks/RESULTS.md) — методология замеров.

---

## Благодарности

- **[EvilBeaver](https://github.com/EvilBeaver) и команда OneScript** — за открытый и хорошо структурированный код. Без `oscript -compile` и доступа к исходникам PoC занял бы кратно больше времени.
- **[Haoran Xu (sillycross)](https://sillycross.github.io/)** — за работу над copy-and-patch compilation и Deegen.
- **[Fredrik Kjolstad](https://fredrikbk.com/)** — соавтор оригинальной статьи "Copy-and-Patch Compilation" (OOPSLA 2021).

---

## Лицензия

[Apache 2.0](LICENSE).

Совместима с MPL OneScript (мы используем только публичные интерфейсы апстрима + дамп bytecode через `CompilerFrontend.Compile`).

---

## Хочешь обсудить / поучаствовать?

Issues / PRs welcome. Особенно интересны:

- Тесты на разнообразных `.os` скриптах из реальных проектов (без stdlib зависимостей).
- Phase 2 — кто-то готов заняться Linux Docker + Deegen integration?
- Benchmarks на других задачах: tight loops, Mandelbrot, n-body.
- Идеи как лучше обыграть пич перед EvilBeaver / OneScript-сообществом.
