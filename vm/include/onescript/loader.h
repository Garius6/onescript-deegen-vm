#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "module.h"

namespace onescript {

class LoaderError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

std::unique_ptr<Module> load_osbin(const std::string& path);

}  // namespace onescript
