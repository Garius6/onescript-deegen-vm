#!/usr/bin/env bash
# Microbenchmark: same .os script run via upstream oscript vs our stub VM.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if ! command -v dotnet >/dev/null 2>&1; then
    echo "skip: .NET SDK not installed" >&2
    exit 77
fi

UPSTREAM="$ROOT/research/upstream/src/oscript/bin/Release/net8.0/oscript.dll"
if [[ ! -f "$UPSTREAM" ]]; then
    echo "building upstream oscript"
    dotnet build "$ROOT/research/upstream/src/oscript" -c Release --nologo >/dev/null
fi

OSBC="$ROOT/tools/osbc-export"
dotnet build "$OSBC" -c Release --nologo >/dev/null

VM="$ROOT/vm/build/osvm"
make -C "$ROOT/vm" build/osvm >/dev/null

for SCRIPT in "$@"; do
    SCRIPT_NAME="$(basename "$SCRIPT" .os)"
    BIN="$ROOT/benchmarks/$SCRIPT_NAME.osbin"
    dotnet run --project "$OSBC" -c Release --no-build -- "$SCRIPT" "$BIN" >/dev/null

    echo "================ $SCRIPT_NAME ================"
    echo "--- upstream oscript (CLR + JIT) ---"
    time dotnet "$UPSTREAM" "$SCRIPT" >/dev/null

    echo "--- onescript-vm stub (C++, no JIT) ---"
    time "$VM" "$BIN" >/dev/null

    echo
done
