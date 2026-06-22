#pragma once

#include <cstdint>
#include <string>

namespace onescript {

enum class ValueKind : uint8_t {
    Undefined,
    Number,
    Boolean,
    String,
};

class Value {
public:
    Value() : kind_(ValueKind::Undefined), num_(0) {}

    static Value Undefined() { return Value(); }
    static Value Number(double n) {
        Value v;
        v.kind_ = ValueKind::Number;
        v.num_  = n;
        return v;
    }
    static Value Boolean(bool b) {
        Value v;
        v.kind_ = ValueKind::Boolean;
        v.num_  = b ? 1.0 : 0.0;
        return v;
    }
    static Value String(std::string s) {
        Value v;
        v.kind_ = ValueKind::String;
        v.str_  = std::move(s);
        return v;
    }

    ValueKind          kind() const { return kind_; }
    double             asNumber() const { return num_; }
    bool               asBoolean() const { return num_ != 0.0; }
    const std::string& asString() const { return str_; }

    bool isTruthy() const;
    std::string toDisplayString() const;

private:
    ValueKind   kind_;
    double      num_;
    std::string str_;
};

}  // namespace onescript
