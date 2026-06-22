// Bytecode disassembler — debugging aid for stub VM bring-up.

#include <cstdio>
#include <cstdlib>

#include "onescript/loader.h"

using namespace onescript;

static const char* kindName(ScopeBindingKind k) {
    switch (k) {
        case ScopeBindingKind::Static:     return "Static";
        case ScopeBindingKind::ThisScope:  return "ThisScope";
        case ScopeBindingKind::FrameScope: return "FrameScope";
    }
    return "?";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: osdis <module.osbin>\n");
        return 2;
    }
    auto mod = load_osbin(argv[1]);

    std::printf(".version %u\n", mod->version);
    std::printf(".entryMethodRef %d\n", mod->entryMethod);

    std::printf(".constants %zu\n", mod->constants.size());
    for (size_t i = 0; i < mod->constants.size(); ++i) {
        std::printf("  %3zu: type=%u %s\n",
                    i, static_cast<unsigned>(mod->constants[i].type),
                    mod->constants[i].presentation.c_str());
    }

    std::printf(".identifiers %zu\n", mod->identifiers.size());
    for (size_t i = 0; i < mod->identifiers.size(); ++i) {
        std::printf("  %3zu: %s\n", i, mod->identifiers[i].c_str());
    }

    std::printf(".variableRefs %zu\n", mod->variableRefs.size());
    for (size_t i = 0; i < mod->variableRefs.size(); ++i) {
        const auto& b = mod->variableRefs[i];
        std::printf("  %3zu: kind=%s scope=%d member=%d target=\"%s\"\n",
                    i, kindName(b.kind), b.scopeIndex, b.memberNumber, b.targetName.c_str());
    }

    std::printf(".methodRefs %zu\n", mod->methodRefs.size());
    for (size_t i = 0; i < mod->methodRefs.size(); ++i) {
        const auto& b = mod->methodRefs[i];
        std::printf("  %3zu: kind=%s scope=%d member=%d target=\"%s\"\n",
                    i, kindName(b.kind), b.scopeIndex, b.memberNumber, b.targetName.c_str());
    }

    std::printf(".methods %zu\n", mod->methods.size());
    for (size_t i = 0; i < mod->methods.size(); ++i) {
        const auto& m = mod->methods[i];
        std::printf("  %3zu: %s (%s) args=%d entry=%d locals=%zu\n",
                    i, m.name.c_str(),
                    m.isFunction ? "Func" : "Proc",
                    m.argCount, m.entryPoint, m.localNames.size());
        for (size_t j = 0; j < m.params.size(); ++j) {
            std::printf("       param[%zu] byval=%d hasDefault=%d defIdx=%d\n",
                        j, m.params[j].isByValue, m.params[j].hasDefault,
                        m.params[j].defaultIndex);
        }
        for (size_t j = 0; j < m.localNames.size(); ++j) {
            std::printf("       local[%zu] %s\n", j, m.localNames[j].c_str());
        }
    }

    std::printf(".code %zu\n", mod->code.size());
    for (size_t i = 0; i < mod->code.size(); ++i) {
        const auto& c = mod->code[i];
        std::printf("  %4zu: %-16s %d\n",
                    i, opcode_name(c.opcode), c.arg);
    }
    return 0;
}
