#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "onescript_opcodes.h"

namespace onescript {

enum class DataType : uint8_t {
    Undefined = 0,
    String    = 1,
    Number    = 2,
    Date      = 3,
    Boolean   = 4,
    Null      = 5,
};

struct Constant {
    DataType    type;
    std::string presentation;
};

enum class ScopeBindingKind : uint8_t {
    Static     = 0,
    ThisScope  = 1,
    FrameScope = 2,
};

struct SymbolBinding {
    ScopeBindingKind kind;
    int32_t          scopeIndex;
    int32_t          memberNumber;
    std::string      targetName;
};

struct Param {
    bool    isByValue;
    bool    hasDefault;
    int32_t defaultIndex;
};

struct Method {
    std::string              name;
    bool                     isFunction;
    int32_t                  argCount;
    std::vector<Param>       params;
    int32_t                  entryPoint;
    std::vector<std::string> localNames;
};

struct Command {
    OpCode  opcode;
    int32_t arg;
};

struct Module {
    uint16_t                   version{1};
    int32_t                    entryMethod{-1};
    std::vector<Constant>      constants;
    std::vector<std::string>   identifiers;
    std::vector<SymbolBinding> variableRefs;
    std::vector<SymbolBinding> methodRefs;
    std::vector<Method>        methods;
    std::vector<Command>       code;
};

}  // namespace onescript
