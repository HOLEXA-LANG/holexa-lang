#!/bin/bash
HLXC="../compiler/hlxc"
PASS=0; FAIL=0
run_test() {
    local name="$1" file="$2" expected="$3"
    $HLXC "$file" 2>/dev/null
    bin="${file%.hlx}"
    result=$("$bin" 2>/dev/null)
    rm -f "$bin"
    if echo "$result" | grep -qF "$expected" 2>/dev/null || [ "$result" = "$expected" ]; then
        echo "  ✅ $name"; PASS=$((PASS+1))
    else
        echo "  ❌ $name — Expected:'$expected' Got:'$result'"; FAIL=$((FAIL+1))
    fi
}
echo "Running HOLEXA Test Suite..."
run_test "hello world"  compiler/hello_test.hlx  "Hello"
echo "Passed: $PASS  Failed: $FAIL"
