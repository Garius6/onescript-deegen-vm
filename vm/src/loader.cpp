#include "onescript/loader.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

namespace onescript {

namespace {

constexpr uint16_t kMaxSupportedVersion = 1;

class Reader {
public:
    Reader(const uint8_t* data, size_t size) : data_(data), size_(size), pos_(0) {}

    void require(size_t n) {
        if (pos_ + n > size_) {
            throw LoaderError("unexpected end of .osbin");
        }
    }

    uint8_t  u8()  { require(1); return data_[pos_++]; }
    uint16_t u16() { require(2); uint16_t v; std::memcpy(&v, data_+pos_, 2); pos_+=2; return v; }
    uint32_t u32() { require(4); uint32_t v; std::memcpy(&v, data_+pos_, 4); pos_+=4; return v; }
    int32_t  i32() { return static_cast<int32_t>(u32()); }
    bool     boolean() { return u8() != 0; }

    std::string lstring() {
        uint32_t len = u32();
        require(len);
        std::string s(reinterpret_cast<const char*>(data_ + pos_), len);
        pos_ += len;
        return s;
    }

    void magic(const char* expected) {
        require(4);
        if (std::memcmp(data_ + pos_, expected, 4) != 0) {
            throw LoaderError("bad magic in .osbin");
        }
        pos_ += 4;
    }

private:
    const uint8_t* data_;
    size_t         size_;
    size_t         pos_;
};

SymbolBinding read_binding(Reader& r) {
    SymbolBinding b;
    b.kind         = static_cast<ScopeBindingKind>(r.u8());
    b.scopeIndex   = r.i32();
    b.memberNumber = r.i32();
    b.targetName   = r.lstring();
    return b;
}

}  // namespace

std::unique_ptr<Module> load_osbin(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw LoaderError("cannot open " + path);
    }
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)), {});
    Reader r(buf.data(), buf.size());

    auto mod = std::make_unique<Module>();

    r.magic("OSBN");
    mod->version = r.u16();
    if (mod->version > kMaxSupportedVersion) {
        throw LoaderError("unsupported .osbin version");
    }
    r.u16();                            // flags
    mod->entryMethod = r.i32();

    // Constants
    uint32_t cnt = r.u32();
    mod->constants.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i) {
        Constant c;
        c.type         = static_cast<DataType>(r.u8());
        c.presentation = r.lstring();
        mod->constants.push_back(std::move(c));
    }

    // Identifiers
    cnt = r.u32();
    mod->identifiers.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i) {
        mod->identifiers.push_back(r.lstring());
    }

    // Variable refs
    cnt = r.u32();
    mod->variableRefs.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i) {
        mod->variableRefs.push_back(read_binding(r));
    }

    // Method refs
    cnt = r.u32();
    mod->methodRefs.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i) {
        mod->methodRefs.push_back(read_binding(r));
    }

    // Methods
    cnt = r.u32();
    mod->methods.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i) {
        Method m;
        m.name       = r.lstring();
        m.isFunction = r.boolean();
        m.argCount   = r.i32();
        uint32_t pc  = r.u32();
        m.params.reserve(pc);
        for (uint32_t j = 0; j < pc; ++j) {
            Param p;
            p.isByValue    = r.boolean();
            p.hasDefault   = r.boolean();
            p.defaultIndex = r.i32();
            m.params.push_back(p);
        }
        m.entryPoint   = r.i32();
        uint32_t lc    = r.u32();
        m.localNames.reserve(lc);
        for (uint32_t j = 0; j < lc; ++j) {
            m.localNames.push_back(r.lstring());
        }
        mod->methods.push_back(std::move(m));
    }

    // Code
    cnt = r.u32();
    mod->code.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i) {
        Command cmd;
        cmd.opcode = static_cast<OpCode>(r.u8());
        cmd.arg    = r.i32();
        mod->code.push_back(cmd);
    }

    // checksum — пока пропускаем, не валидируем
    return mod;
}

}  // namespace onescript
