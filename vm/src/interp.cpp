// Stub stack-based interpreter — bootstrap pipeline for the PoC.
// Replaced by Deegen-generated VM in the JIT phase.

#include "onescript/interp.h"

#include <charconv>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace onescript {

namespace {

Value materialize_constant(const Constant& c) {
    switch (c.type) {
        case DataType::Undefined:
        case DataType::Null:
            return Value::Undefined();
        case DataType::Boolean:
            return Value::Boolean(c.presentation == "True" ||
                                  c.presentation == "true" ||
                                  c.presentation == "Истина");
        case DataType::Number: {
            double d = 0.0;
            const char* first = c.presentation.data();
            const char* last  = first + c.presentation.size();
            auto [ptr, ec] = std::from_chars(first, last, d);
            (void)ptr;
            if (ec != std::errc()) {
                throw InterpError("invalid number constant: " + c.presentation);
            }
            return Value::Number(d);
        }
        case DataType::String:
            return Value::String(c.presentation);
        case DataType::Date:
            throw InterpError("Date constants not supported in PoC");
    }
    return Value::Undefined();
}

struct Frame {
    const Method*      method;
    int32_t            ip;
    std::vector<Value> locals;
    size_t             stackBase;
    bool               discardReturn = false;
};

class Interp {
public:
    explicit Interp(const Module& mod) : mod_(mod) {}

    Value run() {
        const Method* entry = resolve_entry();
        if (entry == nullptr) {
            return Value::Undefined();
        }
        push_frame(*entry, {});
        execute();
        return result_;
    }

    // Number of "inherited" methods on the script-driven object. Upstream's
    // ScriptDrivenObject biases MemberNumber by GetOwnMethodCount(); for a bare
    // script-only object it is 1. PoC hardcodes this.
    static constexpr int kScriptMethodBias = 1;

    const Method* resolve_entry() {
        // ModuleBody = Methods[MethodRefs[EntryMethodIndex].MemberNumber] — direct, no bias.
        if (mod_.entryMethod < 0) return nullptr;
        if (!mod_.methodRefs.empty()) {
            if (mod_.entryMethod >= static_cast<int32_t>(mod_.methodRefs.size())) {
                return nullptr;
            }
            int idx = mod_.methodRefs[mod_.entryMethod].memberNumber;
            if (idx < 0 || idx >= static_cast<int32_t>(mod_.methods.size())) {
                return nullptr;
            }
            return &mod_.methods[idx];
        }
        if (mod_.entryMethod >= static_cast<int32_t>(mod_.methods.size())) {
            return nullptr;
        }
        return &mod_.methods[mod_.entryMethod];
    }

    int resolve_script_method(int memberNumber) {
        // ThisScope-bound refs use MemberNumber + bias. See ScriptDrivenObject.GetMethodDescriptorIndex.
        return memberNumber - kScriptMethodBias;
    }

private:
    void push_frame(const Method& m, std::vector<Value> args) {
        Frame f;
        f.method    = &m;
        f.ip        = m.entryPoint;
        f.locals.resize(m.localNames.size());
        for (size_t i = 0; i < args.size() && i < f.locals.size(); ++i) {
            f.locals[i] = std::move(args[i]);
        }
        f.stackBase = stack_.size();
        frames_.push_back(std::move(f));
    }

    Frame& frame() { return frames_.back(); }

    void push(Value v) { stack_.push_back(std::move(v)); }
    Value pop() {
        if (stack_.empty()) throw InterpError("stack underflow");
        Value v = std::move(stack_.back());
        stack_.pop_back();
        return v;
    }

    void execute() {
        // Safety cap to avoid runaway loops while the stub interpreter is incomplete.
        // fib(30) ~33M instructions; raise to 1B which is enough for benchmarks.
        constexpr uint64_t kMaxInstructions = 1'000'000'000ULL;
        uint64_t budget = kMaxInstructions;
        while (!frames_.empty()) {
            if (--budget == 0) {
                throw InterpError("instruction budget exhausted (likely infinite loop)");
            }
            Frame& f = frame();
            if (f.ip < 0) {
                throw InterpError("ip negative");
            }
            // Clean fall-off at the end of the method body — pop frame as if Return Undefined was emitted.
            if (f.ip >= static_cast<int32_t>(mod_.code.size())) {
                Frame done = std::move(frames_.back());
                frames_.pop_back();
                stack_.resize(done.stackBase);
                if (frames_.empty()) {
                    result_ = Value::Undefined();
                }
                continue;
            }
            const Command& cmd = mod_.code[f.ip];
            switch (cmd.opcode) {
                case OpCode::Nop:
                case OpCode::LineNum:
                    ++f.ip;
                    break;

                case OpCode::PushInt:
                    push(Value::Number(cmd.arg));
                    ++f.ip;
                    break;

                case OpCode::PushConst:
                    push(materialize_constant(mod_.constants.at(cmd.arg)));
                    ++f.ip;
                    break;

                case OpCode::PushUndef:
                    push(Value::Undefined());
                    ++f.ip;
                    break;

                case OpCode::PushBool:
                    push(Value::Boolean(cmd.arg != 0));
                    ++f.ip;
                    break;

                case OpCode::PushLoc:
                    push(f.locals.at(cmd.arg));
                    ++f.ip;
                    break;

                case OpCode::PushVar: {
                    int slot = cmd.arg;
                    if (slot < 0) throw InterpError("PushVar negative slot");
                    if (static_cast<size_t>(slot) >= globals_.size()) {
                        globals_.resize(slot + 1);
                    }
                    push(globals_[slot]);
                    ++f.ip;
                    break;
                }

                case OpCode::LoadVar: {
                    // Store top-of-stack into module-level global slot.
                    Value v = pop();
                    int slot = cmd.arg;
                    if (slot < 0) throw InterpError("LoadVar negative slot");
                    if (static_cast<size_t>(slot) >= globals_.size()) {
                        globals_.resize(slot + 1);
                    }
                    globals_[slot] = std::move(v);
                    ++f.ip;
                    break;
                }

                case OpCode::LoadLoc: {
                    // Upstream naming is inverted: LoadLoc = STORE into local.
                    Value v = pop();
                    if (cmd.arg < 0) throw InterpError("LoadLoc negative slot");
                    if (static_cast<size_t>(cmd.arg) >= f.locals.size()) {
                        f.locals.resize(cmd.arg + 1);
                    }
                    f.locals[cmd.arg] = std::move(v);
                    ++f.ip;
                    break;
                }

                case OpCode::AssignRef: {
                    Value v   = pop();
                    Value ref = pop();
                    // For PoC: AssignRef expects a "ref" wrapper. Stub treats top-1 as local slot index.
                    int slot = static_cast<int>(ref.asNumber());
                    f.locals.at(slot) = std::move(v);
                    ++f.ip;
                    break;
                }

                case OpCode::Add: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Number(a.asNumber() + b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::Sub: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Number(a.asNumber() - b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::Mul: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Number(a.asNumber() * b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::Div: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Number(a.asNumber() / b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::Mod: {
                    Value b = pop();
                    Value a = pop();
                    double r = std::fmod(a.asNumber(), b.asNumber());
                    push(Value::Number(r));
                    ++f.ip;
                    break;
                }
                case OpCode::Neg: {
                    Value a = pop();
                    push(Value::Number(-a.asNumber()));
                    ++f.ip;
                    break;
                }

                case OpCode::Equals: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Boolean(a.asNumber() == b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::NotEqual: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Boolean(a.asNumber() != b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::Less: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Boolean(a.asNumber() < b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::LessOrEqual: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Boolean(a.asNumber() <= b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::Greater: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Boolean(a.asNumber() > b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::GreaterOrEqual: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Boolean(a.asNumber() >= b.asNumber()));
                    ++f.ip;
                    break;
                }
                case OpCode::Not: {
                    Value a = pop();
                    push(Value::Boolean(!a.isTruthy()));
                    ++f.ip;
                    break;
                }
                case OpCode::And: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Boolean(a.isTruthy() && b.isTruthy()));
                    ++f.ip;
                    break;
                }
                case OpCode::Or: {
                    Value b = pop();
                    Value a = pop();
                    push(Value::Boolean(a.isTruthy() || b.isTruthy()));
                    ++f.ip;
                    break;
                }

                case OpCode::Jmp:
                    f.ip = cmd.arg;
                    break;
                case OpCode::JmpFalse: {
                    Value c = pop();
                    if (!c.isTruthy()) {
                        f.ip = cmd.arg;
                    } else {
                        ++f.ip;
                    }
                    break;
                }
                case OpCode::Inc: {
                    Value v = pop();
                    push(Value::Number(v.asNumber() + 1.0));
                    ++f.ip;
                    break;
                }

                case OpCode::CallFunc:
                case OpCode::CallProc: {
                    const SymbolBinding& ref = mod_.methodRefs.at(cmd.arg);
                    bool wantsValue = cmd.opcode == OpCode::CallFunc;

                    // Top of stack is ArgNum count pushed by ArgNum opcode.
                    int nargs = pop_argnum();

                    std::vector<Value> args(nargs);
                    for (int i = nargs - 1; i >= 0; --i) {
                        args[i] = pop();
                    }

                    if (ref.kind == ScopeBindingKind::Static) {
                        call_builtin(ref, std::move(args), wantsValue);
                        ++f.ip;
                        break;
                    }

                    int methodIdx = resolve_script_method(ref.memberNumber);
                    if (methodIdx < 0 ||
                        methodIdx >= static_cast<int32_t>(mod_.methods.size())) {
                        throw InterpError("script method index out of range");
                    }
                    const Method& m = mod_.methods[methodIdx];

                    ++f.ip;
                    push_frame(m, std::move(args));
                    frames_.back().discardReturn = !wantsValue;
                    break;
                }

                case OpCode::ArgNum:
                    push(Value::Number(cmd.arg));
                    ++f.ip;
                    break;

                case OpCode::MakeRawValue:
                case OpCode::MakeBool:
                    // No-op for PoC: values already in canonical form.
                    ++f.ip;
                    break;

                case OpCode::PushTmp: {
                    Value v = pop();
                    tmps_.push_back(v);
                    push(v);
                    ++f.ip;
                    break;
                }

                case OpCode::PopTmp:
                    if (tmps_.empty()) throw InterpError("PopTmp on empty tmps");
                    if (cmd.arg != 0) {
                        // Argument typically: 0 = discard, 1 = restore. Treat any non-zero as restore.
                        push(tmps_.back());
                    }
                    tmps_.pop_back();
                    ++f.ip;
                    break;

                case OpCode::Return: {
                    Value ret = stack_.empty() ? Value::Undefined() : pop();
                    Frame done = std::move(frames_.back());
                    frames_.pop_back();
                    stack_.resize(done.stackBase);
                    if (frames_.empty()) {
                        result_ = std::move(ret);
                    } else if (done.method->isFunction && !done.discardReturn) {
                        push(std::move(ret));
                    }
                    break;
                }

                default:
                    throw InterpError(std::string("opcode not implemented in stub: ") +
                                      opcode_name(cmd.opcode));
            }
        }
    }

    int pop_argnum() {
        if (stack_.empty() || stack_.back().kind() != ValueKind::Number) {
            throw InterpError("expected ArgNum marker on top of stack before call");
        }
        int n = static_cast<int>(stack_.back().asNumber());
        stack_.pop_back();
        return n;
    }

    void call_builtin(const SymbolBinding& ref, std::vector<Value> args, bool wantsValue) {
        const std::string& target = ref.targetName;
        // PoC builtin dispatch: SystemGlobalContext member 0 is Сообщить in upstream.
        // Real impl needs a generated mapping; here we treat any GlobalContext call with args
        // as a print.
        if (target.find("GlobalContext") != std::string::npos && !args.empty()) {
            std::printf("%s\n", args[0].toDisplayString().c_str());
        }
        if (wantsValue) {
            push(Value::Undefined());
        }
    }

    const Module&      mod_;
    std::vector<Frame> frames_;
    std::vector<Value> stack_;
    std::vector<Value> tmps_;
    std::vector<Value> globals_;
    Value              result_;
};

}  // namespace

Value run_main(const Module& mod) {
    Interp i(mod);
    return i.run();
}

}  // namespace onescript
