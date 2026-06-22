#include <cassert>
#include <cstdio>

#include "onescript/interp.h"

using namespace onescript;

// Hand-rolled module for fib(10).
//
// Equivalent to:
//   Функция Фиб(н)
//       Если н < 2 Тогда
//           Возврат н;
//       КонецЕсли;
//       Возврат Фиб(н - 1) + Фиб(н - 2);
//   КонецФункции
//   Возврат Фиб(10);
//
// Method 0 = Фиб(н)
// Method 1 = Main (no args)
// methodRefs[0] -> method 0 (Фиб)
// methodRefs[1] -> method 1 (Main)
static Module make_fib_module(int n) {
    Module m;

    // Method "Фиб"
    Method fib;
    fib.name       = "Фиб";
    fib.isFunction = true;
    fib.argCount   = 1;
    fib.params.push_back({true, false, -1});
    fib.entryPoint = 0;
    fib.localNames = {"н"};

    // Method "Main"
    Method main;
    main.name       = "Main";
    main.isFunction = true;
    main.argCount   = 0;
    main.entryPoint = 100;          // filled below

    m.methods = {fib, main};
    m.entryMethod = 1;

    // methodRefs[0]: ThisScope bias=1 -> methods[0]=Фиб (used by CallFunc 0)
    SymbolBinding callFib;
    callFib.kind         = ScopeBindingKind::ThisScope;
    callFib.scopeIndex   = -1;
    callFib.memberNumber = 1;
    callFib.targetName   = "";
    m.methodRefs.push_back(callFib);
    // methodRefs[1]: entry ref — direct index into Methods (no bias)
    SymbolBinding entryRef;
    entryRef.kind         = ScopeBindingKind::ThisScope;
    entryRef.scopeIndex   = -1;
    entryRef.memberNumber = 1;          // -> methods[1] = Main
    entryRef.targetName   = "";
    m.methodRefs.push_back(entryRef);

    // Фиб (entry=0): ArgNum is required before each CallFunc.
    m.code = {
        {OpCode::PushLoc,  0},
        {OpCode::PushInt,  2},
        {OpCode::Less,     0},
        {OpCode::JmpFalse, 7},
        {OpCode::PushLoc,  0},
        {OpCode::Return,   0},
        {OpCode::Jmp,      7},
        {OpCode::PushLoc,  0},  // 7
        {OpCode::PushInt,  1},
        {OpCode::Sub,      0},
        {OpCode::ArgNum,   1},
        {OpCode::CallFunc, 0},
        {OpCode::PushLoc,  0},
        {OpCode::PushInt,  2},
        {OpCode::Sub,      0},
        {OpCode::ArgNum,   1},
        {OpCode::CallFunc, 0},
        {OpCode::Add,      0},
        {OpCode::Return,   0},
    };

    while (m.code.size() < 100) {
        m.code.push_back({OpCode::Nop, 0});
    }
    // Main(): PushInt n; ArgNum 1; CallFunc 0; Return.
    m.code.push_back({OpCode::PushInt,  n});
    m.code.push_back({OpCode::ArgNum,   1});
    m.code.push_back({OpCode::CallFunc, 0});
    m.code.push_back({OpCode::Return,   0});

    return m;
}

int main() {
    Module mod = make_fib_module(10);
    Value  v   = run_main(mod);
    assert(v.kind() == ValueKind::Number);
    std::printf("fib(10) = %s\n", v.toDisplayString().c_str());
    assert(v.asNumber() == 55.0);
    std::printf("test_fib OK\n");
    return 0;
}
