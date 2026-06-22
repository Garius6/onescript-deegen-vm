#!/usr/bin/env bash
# End-to-end smoke: .os -> .osbin -> stub VM -> expected output.
# Requires .NET 8 SDK (для osbc-export).

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
FIXTURES="$ROOT/tests/fixtures"
mkdir -p "$FIXTURES"

OSBC_DIR="$ROOT/tools/osbc-export"
VM_DIR="$ROOT/vm"

if ! command -v dotnet >/dev/null 2>&1; then
    echo "skip: .NET SDK not installed (osbc-export не собирается)" >&2
    exit 77   # autotools-style "skipped"
fi

echo "==> build osbc-export"
dotnet build "$OSBC_DIR" -c Release --nologo

echo "==> compile fib.os -> fib.osbin"
dotnet run --project "$OSBC_DIR" -c Release -- \
    "$ROOT/tests/e2e/fib.os" \
    "$FIXTURES/fib.osbin"

echo "==> build VM"
make -C "$VM_DIR" build/osvm

echo "==> run fib.osbin"
RESULT="$("$VM_DIR/build/osvm" "$FIXTURES/fib.osbin")"
echo "result: $RESULT"

if [[ "$RESULT" == "55" ]]; then
    echo "PASS"
    exit 0
else
    echo "FAIL: expected 55"
    exit 1
fi
