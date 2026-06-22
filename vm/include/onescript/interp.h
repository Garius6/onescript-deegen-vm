#pragma once

#include "module.h"
#include "value.h"

namespace onescript {

class InterpError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

Value run_main(const Module& mod);

}  // namespace onescript
