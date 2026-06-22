# osbc-export

Bootstrap утилита. Принимает `.os` файл, прогоняет через апстрим OneScript-компилятор и пишет `.osbin` (наш простой формат, см. `docs/osbin-format.md`).

## Сборка

Требуется **.NET 8 SDK** + клонированный апстрим в `research/upstream/`.

```
cd tools/osbc-export
dotnet build
```

## Запуск

```
dotnet run --project tools/osbc-export -- input.os output.osbin
```

Или после `dotnet publish` — нативный бинарь.

## Зависимости

Ссылается на проекты апстрима через `<ProjectReference>` (path-based):

- `ScriptEngine`
- `ScriptEngine.HostedScript`
- `OneScript.Core`
- `OneScript.Language`

## Регенерация opcode mapping

Файл `OpCodes.g.cs` генерируется из `tools/opcodes.csv`:

```
bash scripts/gen_opcode_headers.sh
```

Запускать каждый раз когда апстрим меняет `OperationCode` enum.
