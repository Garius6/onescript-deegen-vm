#include <cstdio>
#include <cstdlib>

#include "onescript/interp.h"
#include "onescript/loader.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: osvm <module.osbin>\n");
        return 2;
    }
    try {
        auto mod   = onescript::load_osbin(argv[1]);
        auto value = onescript::run_main(*mod);
        if (value.kind() != onescript::ValueKind::Undefined) {
            std::printf("%s\n", value.toDisplayString().c_str());
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
