# OneScript Bytecode — Research

Анализ stack VM OneScript (commit на момент `git clone` от `develop`). Цель —
понять что нам нужно загружать в Deegen-VM для PoC.

## TL;DR

- **Bytecode**: stack-based, 147 опкодов, фиксированный формат `Command = (OperationCode Code, int Argument)`.
- **Модуль**: `StackRuntimeModule` — простая структура с пулами констант, идентификаторов, кодом, методами, ref-таблицами.
- **Сериализация**: .NET `BinaryFormatter` (`[Serializable]`) — deprecated в .NET 5+, проприетарный CLR-формат. Не годится для C++ loader.
- **Решение**: написать **bootstrap C#-утилиту** которая грузит модуль через CompilerFrontend и пишет наш простой бинарный формат.
- **Dispatch в апстриме**: function-pointer table `_commands[(int)code](arg)` — медленный. Deegen уделает.

## Файлы апстрима

| Что | Где |
|-----|-----|
| Enum опкодов | `src/ScriptEngine/Machine/Core.cs` (lines 12-142) |
| `Command` struct | `src/ScriptEngine/Machine/Core.cs` (lines 144-154) |
| `DataType`, `ConstDefinition` | `src/ScriptEngine/Machine/Core.cs` (lines 156-183) |
| Модуль | `src/ScriptEngine/Machine/StackRuntimeModule.cs` |
| Executor wrapper | `src/ScriptEngine/Machine/StackMachineExecutor.cs` |
| Interpreter loop | `src/ScriptEngine/Machine/MachineInstance.cs` (line 478, `MainCommandLoop`) |
| ApplicationDump (контейнер) | `src/ScriptEngine/Compiler/ApplicationDump.cs` |
| UserAddedScript | `src/ScriptEngine/Libraries/UserAddedScript.cs` (упрощённая) + расширенная в `ScriptEngine.HostedScript` |
| Текстовый dump (отладка) | `src/ScriptEngine/Compiler/ModuleDumpWriter.cs` |
| Бинарная (де)сериализация | `src/StandaloneRunner/ProcessLoader.cs` |

## Опкоды

147 значений в `enum OperationCode`. Делятся на две категории:

### Core VM (~55 опкодов)

```
Nop, PushVar, PushConst, PushInt, PushBool, PushUndef, PushNull,
PushLoc, PushRef, LoadVar, LoadLoc, AssignRef,
Add, Sub, Mul, Div, Mod, Neg,
Equals, Less, Greater, LessOrEqual, GreaterOrEqual, NotEqual,
Not, And, Or,
CallFunc, CallProc, ArgNum, PushDefaultArg,
ResolveProp, ResolveMethodProc, ResolveMethodFunc,
Jmp, JmpFalse, PushIndexed, Return, JmpCounter, Inc,
NewInstance, NewFunc, PushIterator, IteratorNext, StopIterator,
BeginTry, EndTry, RaiseException, LineNum,
MakeRawValue, MakeBool, PushTmp, PopTmp,
Execute, AddHandler, RemoveHandler, ExitTry
```

### Builtin-функции как опкоды (~85)

`Eval`, `Bool`, `Number`, `Str`, `Date`, `Type`, `ValType`, `StrLen`, `TrimL/R/LR`,
`Left`, `Right`, `Mid`, `StrPos`, `UCase`, `LCase`, `TCase`, `Chr`, `ChrCode`,
`EmptyStr`, `StrReplace`, `StrGetLine`, `StrLineCount`, `StrEntryCount`,
date manipulation (`Year`...`EndOfQuarter`, `AddMonth`, `CurrentDate`),
math (`Round`, `Log`, `Log10`, `Sin`, `Cos`, `Tan`, `ASin`, `ACos`, `ATan`,
`Exp`, `Pow`, `Sqrt`, `Min`, `Max`), `Format`, `ExceptionInfo`,
`ExceptionDescr`, `ModuleInfo`.

**Для PoC** реализуем подмножество core VM (~15-20 опкодов) + 1-2 builtin-а
(`Сообщить` через CallProc / `Format` для I/O).

### `Command` структура

```csharp
[Serializable]
public struct Command {
    public OperationCode Code;   // enum (int32)
    public int Argument;         // operand (index в pool / immediate)
}
```

Фиксированный размер — 8 байт на инструкцию (`int32 + int32`). В нашем формате
сожмём до `u8 + i32` = 5 байт, либо оставим 8 для выравнивания.

## Pool-структуры в модуле

`StackRuntimeModule`:

```csharp
public class StackRuntimeModule : IExecutableModule {
    public int EntryMethodIndex { get; set; } = -1;
    public List<BslPrimitiveValue> Constants { get; }        // pool констант
    internal List<string> Identifiers { get; }                // pool имён
    internal List<ModuleSymbolBinding> VariableRefs { get; }  // late-bound vars
    internal List<ModuleSymbolBinding> MethodRefs { get; }    // late-bound methods
    public IList<BslScriptFieldInfo> Fields { get; }          // глобальные поля
    public IList<BslScriptPropertyInfo> Properties { get; }
    public IList<BslScriptMethodInfo> Methods { get; }        // определения процедур/функций
    public List<Command> Code { get; }                        // байткод
    public SourceCode Source { get; set; }                    // для error reporting
}
```

Каждый `Command.Argument` — индекс в один из пулов (зависит от опкода).

### Константы (типы)

```csharp
[Serializable]
public enum DataType {
    Undefined, String, Number, Date, Boolean, Null
}

[Serializable]
public struct ConstDefinition : IEquatable<ConstDefinition> {
    public DataType Type;
    public string Presentation;   // строковое представление, парсится в runtime
}
```

В compile-time константы хранятся как `ConstDefinition` (строки). В runtime
конвертятся в `BslPrimitiveValue`. **Для нашего формата сериализуем как
`ConstDefinition` (type + presentation)** — парсим при загрузке в C++.

### Methods

`MachineMethod` (из `ModuleDumpWriter` видно):
```
Signature:
    Name: string
    IsFunction: bool
    ArgCount: int
    Params[]:  { IsByValue: bool, HasDefaultValue: bool, DefaultValueIndex: int }
    AnnotationsCount: int
    Annotations[]: { Name, Parameters[] }
EntryPoint: int                  // offset в Code
LocalVariables: VariableInfo[]   // имена локалок (для отладки)
```

### Symbol bindings

```csharp
public class ModuleSymbolBinding {
    public ScopeBindingKind Kind;  // Static, ThisScope, FrameScope
    public object Target;          // null для FrameScope
    public int ScopeIndex;
    public int MemberNumber;
}
```

Late-bound символы — резолвятся в runtime через linker.

## Сериализация (current state)

`StandaloneRunner/ProcessLoader.cs`:

```csharp
private static ApplicationDump DeserializeAppDump(Stream sourceStream) {
    var serializer = new BinaryFormatter();
    var appDump = (ApplicationDump) serializer.Deserialize(sourceStream);
    return appDump;
}
```

`ApplicationDump`:
```csharp
[Serializable]
public class ApplicationDump {
    public UserAddedScript[] Scripts { get; set; }
    public ApplicationResource[] Resources { get; set; }
}
```

### Проблема

`BinaryFormatter` — это .NET-проприетарный формат:
- **Deprecated** с .NET 5 (security flag, CVE-magnet)
- Не загружаемо из C++ без CLR-хостинга
- Огромный (граф объектов с типовыми метаданными)

### Решение для PoC

**Bootstrap-утилита `osbc-export`** (C#, .NET, ~300 LOC):

1. Принимает `.os` файл
2. Использует `CompilerFrontend.Compile(source) → StackRuntimeModule`
3. Сериализует в **наш простой бинарный формат** (или JSON для отладки)
4. Output: `module.osbin`

C++ loader читает `.osbin` (BinaryReader-style), не зависит от .NET.

## Наш формат `.osbin` (proposal)

Little-endian, простой бинарь. JSON-вариант для дебага.

```
Header:
    magic:    [4]u8 = "OSBN"
    version:  u16 = 1
    flags:    u16 = 0

Constants pool:
    count:    u32
    items[count]:
        type:         u8 (DataType: 0=Undefined, 1=String, 2=Number, 3=Date, 4=Boolean, 5=Null)
        presentation: lstring     // u32 len + UTF-8 bytes

Identifiers pool:
    count:    u32
    items[count]: lstring

Variable refs:
    count:    u32
    items[count]: ModuleSymbolBinding (kind:u8, scopeIndex:i32, memberNumber:i32, targetName:lstring?)

Method refs:
    count:    u32
    items[count]: ModuleSymbolBinding (same)

Methods:
    count:    u32
    items[count]:
        name:           lstring
        isFunction:     u8
        argCount:       i32
        params:
            count:      u32
            items[count]: { isByValue:u8, hasDefault:u8, defaultIndex:i32 }
        annotations:    (skip для PoC)
        entryPoint:     i32
        localCount:     i32
        localNames[localCount]: lstring  // для отладки

Code:
    count:    u32
    items[count]:
        opcode: u8     // enum OperationCode (mapping таблица в обе стороны)
        arg:    i32

EntryMethodIndex: i32
```

`opcode` сжатый до `u8` — у нас 147 опкодов, влезает. Mapping enum
сгенерим из `Core.cs` (источник истины).

## Interpreter dispatch в апстриме

```csharp
private void MainCommandLoop() {
    while (_currentFrame.InstructionPointer >= 0
           && _currentFrame.InstructionPointer < _module.Code.Count) {
        var command = _module.Code[_currentFrame.InstructionPointer];
        _commands[(int) command.Code](command.Argument);
    }
}
```

- `_commands` — массив делегатов размером ~147
- Каждый opcode = отдельный метод C# на `MachineInstance`
- Опкод-метод сам инкрементит IP / выставляет в jump target
- **Indirect call через делегат** = слот в array + virtual call cost. Без inline-cache.

### Наш baseline (Deegen interpreter)

Threaded code через computed-goto / tail-calls (Deegen генерирует).
Ожидаем **2-4×** speedup только от лучшего диспатча, без JIT.

### Наш target (Deegen baseline JIT)

Copy-and-patch на горячих функциях.
Ожидаем **10-30×** vs упстрим интерпретатор.

## Что НЕ поддерживаем в PoC

| Фича | Почему |
|------|--------|
| Builtin-функции кроме Number/String math | Скоуп |
| Массив, Структура, Соответствие, ТЗ | Объектная модель, отдельная стадия |
| `Новый` (NewInstance) | Нет stdlib типов |
| ResolveProp/ResolveMethod* | Объекты не нужны для compute-бенчмарков |
| BeginTry/EndTry/Raise/ExitTry | Без exceptions для PoC |
| Iterators (PushIterator, IteratorNext, StopIterator) | Циклы `Для i=...` через JmpCounter+Inc хватит |
| Аннотации, события (AddHandler) | Лишнее |
| ApplicationResource | Не наш use-case |

## Что нужно поддержать в PoC (полный список)

**Опкоды** (~20):
```
Nop, PushConst, PushInt, PushBool, PushUndef,
LoadVar, LoadLoc, PushVar, PushLoc,
Add, Sub, Mul, Div, Mod, Neg,
Equals, Less, Greater, LessOrEqual, GreaterOrEqual, NotEqual,
Not, And, Or,
Jmp, JmpFalse, JmpCounter, Inc,
CallFunc, CallProc, Return,
ArgNum, PushDefaultArg,
LineNum  // no-op в нашем VM, для compat
```

**Типы значений** (3): `Число` (double + integer fast-path), `Булево`, `Неопределено`.
`Строка` — добавим если нужна для `Сообщить`.

**Builtins** (1-2): `Сообщить` (печать), опционально `Format`.

## Бенчмарки для демо

| Бенчмарк | Опкоды | Зачем |
|----------|--------|-------|
| `fib(35)` рекурсивный | CallFunc, Return, Less, Sub, Add | Чистый dispatch + call overhead |
| Tight loop `Для i=1 По 100M Цикл s += i` | JmpCounter, Inc, Add | Loop performance |
| `mandel(80x80)` | Add, Sub, Mul, Div, Less, JmpFalse, Jmp | Float math + branches |

Все три **не требуют stdlib типов** — только числа, циклы, функции.

## Следующие шаги (по приоритету)

1. **Дизайн `.osbin` формата** — финализировать выше + кодогенерация enum mapping. → `docs/osbin-format.md`
2. **C#-утилита `osbc-export`** — бутстрап. → `tools/osbc-export/` (.NET project).
3. **Тестовый .os файл** + золотой `.osbin` для unit-теста C++ loader. → `tests/fixtures/`
4. **C++ loader** для `.osbin`. → `vm/loader.cpp`
5. **Deegen-VM**: stencils для 20 опкодов, value type Число. → `vm/bytecode/`
6. **Builtin `Сообщить`** → `vm/builtins/`
7. **Запуск fib(10)** end-to-end. → `tests/e2e/fib.os`
8. **Бенчмарки + замеры** vs `oscript` upstream. → `benchmarks/RESULTS.md`

## Risks / open questions

- **Линейная сигнатура `_commands[(int)code]`**: если Deegen-stencil читает opcode-байт из `[ip]` — выравнивание не проблема. Но если хотим **direct threading** (адрес-инструкции вместо опкода) — нужен этап "линковки" после загрузки. Решим при дизайне stencils.
- **`PushIndexed`, `MakeRawValue`, `MakeBool`** — назначение неочевидно из имени. Прочитать handler в `MachineInstance` перед реализацией.
- **`Inc` + `JmpCounter`** — точная семантика `Для i = a По b` цикла. Прочитать handler.
- **Constants как `BslPrimitiveValue`** в модуле vs `ConstDefinition` в compile-time. Какой именно идёт в pool после компиляции? Скорее всего `BslPrimitiveValue` (готовые runtime values) — но сериализовать удобнее через `ConstDefinition`. Уточнить в bootstrap-утилите.
- **Локализация ошибок**: `LineNum` опкод даёт линию для отчётов. Без него стектрейсы пустые. Реализуем как nop с записью линии в frame.

## Заметка для апстрим-разработчиков

Если PoC получится, формат `.osbin` можно предложить как **второй вариант сериализации** в OneScript (рядом с `BinaryFormatter`):
- Решает проблему deprecated BinaryFormatter
- Открывает дверь альтернативным VM (наш Deegen-VM, потенциально другие)
- Простой формат = легко версионировать и эволюционировать
