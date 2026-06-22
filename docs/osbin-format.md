# `.osbin` Binary Format Specification

Простой бинарный формат для сериализации скомпилированного OneScript-модуля.
Источник истины для C# writer (`osbc-export`) и C++ loader (`vm/loader.cpp`).

## Цели

- Однопроходное чтение, без back-references
- Little-endian, выровнено по байтам
- Версионируемое
- Маленький — bytecode тестового `fib.os` должен быть < 1 KB

## Layout

```
+----------------------+
| Header               |
+----------------------+
| Constants pool       |
+----------------------+
| Identifiers pool     |
+----------------------+
| Variable refs        |
+----------------------+
| Method refs          |
+----------------------+
| Methods              |
+----------------------+
| Code                 |
+----------------------+
| Footer               |
+----------------------+
```

## Типы данных

| Тип | Размер | Описание |
|-----|--------|----------|
| `u8`/`u16`/`u32`/`u64` | 1/2/4/8 байт | Беззнаковые, LE |
| `i8`/`i16`/`i32`/`i64` | 1/2/4/8 байт | Двоичный комплемент, LE |
| `f64` | 8 байт | IEEE 754 binary64 |
| `lstring` | `u32 len + len байт` | UTF-8 без null-терминатора |
| `bool` | `u8` | 0 = false, иначе true |

## Header

```
magic:        [4]u8 = "OSBN"
version:      u16 = 1
flags:        u16 = 0           // зарезервировано
entryMethod:  i32               // индекс в Methods, -1 если нет
```

## Constants pool

```
count:       u32
items[count]:
    type:           u8 (DataType)
    presentation:   lstring     // строковое представление, парсится при загрузке
```

`DataType`:

| Value | Name |
|-------|------|
| 0 | Undefined |
| 1 | String |
| 2 | Number |
| 3 | Date |
| 4 | Boolean |
| 5 | Null |

**Парсинг presentation**:

- `Undefined` / `Null` → presentation игнорируется (пустая строка)
- `Boolean` → `"True"` / `"False"` (case-sensitive)
- `Number` → десятичная строка с `.` как разделителем (`"3.14"`, `"-42"`)
- `String` → UTF-8 строка как есть
- `Date` → ISO-like `"YYYY-MM-DDThh:mm:ss"` (для PoC не нужен)

## Identifiers pool

```
count:       u32
items[count]: lstring
```

Имена переменных, методов, типов — всё что компилятор оставил для late binding.

## Variable refs / Method refs

Обе таблицы одного формата:

```
count:       u32
items[count]:
    kind:         u8 (ScopeBindingKind)
    scopeIndex:   i32
    memberNumber: i32
    targetName:   lstring     // имя для Static binding, иначе ""
```

`ScopeBindingKind`:

| Value | Name |
|-------|------|
| 0 | Static |
| 1 | ThisScope |
| 2 | FrameScope |

## Methods

```
count:       u32
items[count]:
    name:           lstring
    isFunction:     bool       // true = функция, false = процедура
    argCount:       i32
    paramCount:     u32
    params[paramCount]:
        isByValue:     bool
        hasDefault:    bool
        defaultIndex:  i32    // -1 если hasDefault=false
    entryPoint:     i32       // offset в Code[]
    localCount:     u32
    localNames[localCount]: lstring
```

Аннотации не сериализуем в PoC.

## Code

```
count:       u32
items[count]:
    opcode: u8       // см. tools/opcodes.csv
    arg:    i32      // операнд (индекс в pool или immediate)
```

Фиксированный размер инструкции = 5 байт. Можно оптимизировать в будущем
(variable-length encoding), пока — простота.

**Маппинг `OpCode → u8`** генерится скриптом `scripts/gen_opcodes.sh` из
`Core.cs`. Должен совпадать на C# и C++ стороне.

## Footer

```
checksum:    u32     // CRC32 от всех байт от начала header до конца code
                     // (не включая сам checksum)
```

Опциональная sanity-проверка. Loader должен ругаться при mismatch.

## Версионирование

`version` инкрементим при breaking changes. Loader проверяет:

- `version <= MAX_SUPPORTED_VERSION` → читает
- иначе → ошибка `"unsupported .osbin version"`

Текущий `MAX_SUPPORTED_VERSION = 1`.

## Пример (псевдо-hex)

`fib.os` после компиляции:

```
4F 53 42 4E              // magic "OSBN"
01 00                    // version=1
00 00                    // flags=0
00 00 00 00              // entryMethod=0

// Constants: 2 (число 2, число 1)
02 00 00 00
02 01 00 00 00 "2"
02 01 00 00 00 "1"

// Identifiers: 1 ("Фиб")
01 00 00 00
06 00 00 00 D0 A4 D0 B8 D0 B1   // "Фиб" в UTF-8

// VarRefs: 0
00 00 00 00
// MethRefs: 0
00 00 00 00

// Methods: 1 (Фиб)
01 00 00 00
06 00 00 00 D0 A4 D0 B8 D0 B1   // name "Фиб"
01                              // isFunction=true
01 00 00 00                     // argCount=1
01 00 00 00                     // paramCount=1
01 00 FF FF FF FF               // param: byVal, no default
00 00 00 00                     // entryPoint=0
01 00 00 00                     // localCount=1
01 00 00 00 "н"                 // local name

// Code: ...
0A 00 00 00                     // 10 commands
03 02 00 00 00                  // PushInt 2
07 00 00 00 00                  // PushLoc 0 (н)
13 00 00 00 00                  // Less
21 0X 00 00 00                  // JmpFalse -> X
// ... etc

// Footer
DE AD BE EF                     // CRC32
```

## Open items

- **Аннотации**: skip для PoC, добавить если потребуются для линковки
- **Source mapping**: line numbers через `LineNum` опкод хранятся в коде, отдельной таблицы не делаем
- **Multi-module ApplicationDump**: пока один модуль на файл, multi-module добавим если понадобится
