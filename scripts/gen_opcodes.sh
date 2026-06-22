#!/usr/bin/env bash
# Regenerate opcodes.csv from upstream OneScript Core.cs.
# Source of truth for opcode <-> u8 mapping.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CORE="$ROOT/research/upstream/src/ScriptEngine/Machine/Core.cs"

if [[ ! -f "$CORE" ]]; then
    echo "Upstream Core.cs not found at $CORE" >&2
    echo "Run: git clone https://github.com/EvilBeaver/OneScript $ROOT/research/upstream" >&2
    exit 1
fi

awk '/public enum OperationCode/,/^    }$/' "$CORE" \
    | grep -E "^        [A-Z]" \
    | sed 's/^        //; s/,$//; s/ *\/\/.*//' \
    | awk 'BEGIN{print "value,name"; i=0} {printf "%d,%s\n", i, $0; i++}' \
    > "$ROOT/tools/opcodes.csv"

echo "Generated $(wc -l < "$ROOT/tools/opcodes.csv") rows in tools/opcodes.csv"
