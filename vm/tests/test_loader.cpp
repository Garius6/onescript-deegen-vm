#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>

#include "onescript/loader.h"

using namespace onescript;

static void write_u8 (std::vector<uint8_t>& b, uint8_t v)  { b.push_back(v); }
static void write_u16(std::vector<uint8_t>& b, uint16_t v) { b.push_back(v & 0xFF); b.push_back(v >> 8); }
static void write_u32(std::vector<uint8_t>& b, uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back((v >> (8*i)) & 0xFF);
}
static void write_lstring(std::vector<uint8_t>& b, const std::string& s) {
    write_u32(b, static_cast<uint32_t>(s.size()));
    for (char c : s) b.push_back(static_cast<uint8_t>(c));
}

int main() {
    std::vector<uint8_t> buf;
    // header
    buf.push_back('O'); buf.push_back('S'); buf.push_back('B'); buf.push_back('N');
    write_u16(buf, 1);
    write_u16(buf, 0);
    write_u32(buf, 0);  // entryMethod = 0
    // constants: 0
    write_u32(buf, 0);
    // identifiers: 0
    write_u32(buf, 0);
    // varRefs: 0
    write_u32(buf, 0);
    // methodRefs: 0
    write_u32(buf, 0);
    // methods: 1
    write_u32(buf, 1);
    write_lstring(buf, "Main");
    write_u8(buf, 0);              // isFunction=false
    write_u32(buf, 0);             // argCount=0
    write_u32(buf, 0);             // paramCount=0
    write_u32(buf, 0);             // entryPoint=0
    write_u32(buf, 0);             // localCount=0
    // code: 1 (Return)
    write_u32(buf, 1);
    write_u8(buf, 37);             // Return = 37 in opcodes.csv
    write_u32(buf, 0);
    // checksum (ignored)
    write_u32(buf, 0xDEADBEEF);

    const char* path = "test_loader.osbin";
    {
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    }
    auto mod = load_osbin(path);
    assert(mod->version == 1);
    assert(mod->entryMethod == 0);
    assert(mod->methods.size() == 1);
    assert(mod->methods[0].name == "Main");
    assert(mod->code.size() == 1);
    assert(mod->code[0].opcode == OpCode::Return);
    std::printf("test_loader OK\n");
    std::remove(path);
    return 0;
}
