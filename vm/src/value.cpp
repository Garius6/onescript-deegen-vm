#include "onescript/value.h"

#include <cstdio>

namespace onescript {

bool Value::isTruthy() const {
    switch (kind_) {
        case ValueKind::Undefined: return false;
        case ValueKind::Number:    return num_ != 0.0;
        case ValueKind::Boolean:   return num_ != 0.0;
        case ValueKind::String:    return !str_.empty();
    }
    return false;
}

std::string Value::toDisplayString() const {
    switch (kind_) {
        case ValueKind::Undefined: return "Неопределено";
        case ValueKind::Boolean:   return num_ != 0.0 ? "Истина" : "Ложь";
        case ValueKind::String:    return str_;
        case ValueKind::Number: {
            char buf[64];
            if (num_ == static_cast<int64_t>(num_)) {
                std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(num_));
            } else {
                std::snprintf(buf, sizeof(buf), "%g", num_);
            }
            return buf;
        }
    }
    return {};
}

}  // namespace onescript
