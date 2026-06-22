#include <cassert>
#include <cstdio>

#include "onescript/interp.h"

using namespace onescript;

// Hand-rolled module: Main() returns 2 + 3
static Module make_addition_module() {
    Module m;
    m.entryMethod = 0;
    Method main;
    main.name       = "Main";
    main.isFunction = true;
    main.argCount   = 0;
    main.entryPoint = 0;
    m.methods.push_back(main);
    m.code = {
        {OpCode::PushInt, 2},
        {OpCode::PushInt, 3},
        {OpCode::Add, 0},
        {OpCode::Return, 0},
    };
    return m;
}

int main() {
    Module mod = make_addition_module();
    Value  v   = run_main(mod);
    assert(v.kind() == ValueKind::Number);
    assert(v.asNumber() == 5.0);
    std::printf("test_interp OK: 2 + 3 = %s\n", v.toDisplayString().c_str());
    return 0;
}
