#!/usr/bin/env bash
# Generate C++ and C# headers from opcodes.csv.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CSV="$ROOT/tools/opcodes.csv"

if [[ ! -f "$CSV" ]]; then
    echo "$CSV not found. Run gen_opcodes.sh first." >&2
    exit 1
fi

CPP="$ROOT/vm/include/onescript_opcodes.h"
mkdir -p "$(dirname "$CPP")"
{
    echo "// Generated from tools/opcodes.csv. DO NOT EDIT."
    echo "#pragma once"
    echo "#include <cstdint>"
    echo ""
    echo "namespace onescript {"
    echo "enum class OpCode : uint8_t {"
    tail -n +2 "$CSV" | awk -F, '{printf "    %s = %s,\n", $2, $1}'
    echo "};"
    echo ""
    echo "constexpr const char* opcode_name(OpCode op) {"
    echo "    switch (op) {"
    tail -n +2 "$CSV" | awk -F, '{printf "        case OpCode::%s: return \"%s\";\n", $2, $2}'
    echo "        default: return \"<unknown>\";"
    echo "    }"
    echo "}"
    echo ""
    echo "constexpr uint32_t kOpCodeCount = $(($(wc -l < "$CSV") - 1));"
    echo "}  // namespace onescript"
} > "$CPP"
echo "Generated $CPP"

CS="$ROOT/tools/osbc-export/OpCodes.g.cs"
mkdir -p "$(dirname "$CS")"
{
    echo "// Generated from tools/opcodes.csv. DO NOT EDIT."
    echo "namespace OsbcExport;"
    echo ""
    echo "public static class OpCodeMapping"
    echo "{"
    echo "    public static readonly System.Collections.Generic.Dictionary<string, byte> NameToValue = new()"
    echo "    {"
    tail -n +2 "$CSV" | awk -F, '{printf "        [\"%s\"] = %s,\n", $2, $1}'
    echo "    };"
    echo "}"
} > "$CS"
echo "Generated $CS"
