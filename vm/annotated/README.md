# OneScript opcodes in Deegen DSL

Файлы здесь обрабатываются **во время сборки** Deegen meta-compiler-ом из `third_party/deegen/deegen/`. Это не обычный C++ — без Deegen-pipeline-а они НЕ собираются. См. `docs/deegen-integration-plan.md` (Путь B).

## Текущее покрытие (spike)

| Опкод | Семантика | Соответствие OneScript stack-op |
|-------|-----------|---------------------------------|
| `OsLoadInt16` | Положить int16 литерал в slot | `PushInt` |
| `OsMov` | Копировать slot → slot | пара `PushLoc src; LoadLoc dst` после stack-to-register lowering |
| `OsAdd` | `out = lhs + rhs` (double) | `Add` |
| `OsSub` | `out = lhs - rhs` (double) | `Sub` |
| `OsRet` | Вернуть значение(я) из метода | `Return` |

## Что НЕ работает

- **Эти определения ещё не собирались**. Build Deegen в Docker x86_64 идёт в фоне; CMake target для нашей VM не написан.
- **Stack→register translator** для `.osbin` ещё не написан. Без него мы не можем превратить fib.osbin в последовательность OsLoadInt16/OsAdd/OsRet.
- **Frame layout** OneScript (SDO bias, ArgNum опкод, ModuleBody) — не отображён в Deegen-frame model. Это самая нетривиальная часть Path B.

## Следующие шаги

1. Дождаться завершения `python3 ljr-build make release` в `third_party/deegen/`.
2. Сделать CMake target в Deegen build для нашего translation unit (`vm/annotated/onescript_bytecodes.cpp`).
3. Hand-emit-нуть один минимальный CodeBlock (`OsLoadInt16 5; OsLoadInt16 3; OsAdd; OsRet`) из тестового загрузчика. Цель: `5 + 3 = 8` через Deegen interpreter.
4. Замерить производительность Deegen-interpretera vs текущего stub.
5. Включить baseline JIT и снова замерить.
6. Расширять покрытие опкодов до уровня stub (25+ опкодов).
