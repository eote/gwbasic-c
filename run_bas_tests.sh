#!/bin/bash
# GW-BASIC test runner - categorizes tests by feature status

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/tests/bas"
GWBASIC_EXE="$SCRIPT_DIR/gwbasic.exe"

PASS=0
FAIL=0
SKIP=0

# Tests that should PASS (implemented features)
# Updated to match TEST_CATEGORIES.md - removed 6 tests that depend on unimplemented features
IMPLEMENTED_TESTS=(
    "test_hello.bas"
    "test_for.bas"
    "test_if.bas"
    "test_let.bas"
    "test_goto_gosub.bas"
    "test_for_step.bas"
    "test_next_no_var.bas"
    "test_simple.bas"
    "test_scalar.bas"
    "test_empty_stmt.bas"
    "test_print_bare.bas"
    "test_print_hello.bas"
    "test_print_literal.bas"
    "test_basic.bas"
    "test_assign.bas"
    "test_assign2.bas"
    "test_cross_assign.bas"
    "test_expr.bas"
    "test_expr2.bas"
    "test_self_assign.bas"
    "test_simple_assign.bas"
    "test_for2.bas"
    "test_for3.bas"
    "test_int_var.bas"
    "test_arrays.bas"
    "test_multi_array.bas"
    "test_math.bas"
    "test_cond.bas"
    "test_print_file.bas"
    "test_tab_spc.bas"
)

# Tests for NOT YET IMPLEMENTED features (expected to fail)
# Added the 6 tests moved from Implemented: file_seq, line_input, file_rand, put_get, read_data, string_arrays
NOT_IMPLEMENTED_TESTS=(
    "test_def_fn.bas"
    "test_error.bas"
    "test_on_goto.bas"
    "test_string.bas"
    "test_string_funcs.bas"
    "test_string_simple.bas"
    "test_string_var.bas"
    "test_len.bas"
    "test_len2.bas"
    "test_len_literal.bas"
    "test_left.bas"
    "test_left_assign.bas"
    "test_left_direct.bas"
    "test_left_direct2.bas"
    "test_left_literal.bas"
    "test_left_literal2.bas"
    "test_left_only.bas"
    "test_left_print.bas"
    "test_left_simple.bas"
    "test_using_simple.bas"
    "test_using_very_simple.bas"
    "test_print_using.bas"
    "test_while_wend.bas"
    "test_while_debug.bas"
    "test_file_seq.bas"
    "test_line_input.bas"
    "test_file_rand.bas"
    "test_put_get.bas"
    "test_read_data.bas"
    "test_string_arrays.bas"
)

# Run a test and check result
run_test() {
    local test_file=$1
    local expected=$2  # "pass" or "fail"
    local name=$(basename "$test_file" .bas)
    
    echo -n "Testing $name... "
    
    # Run gwbasic.exe directly, capture exit code first (before any pipe)
    "$GWBASIC_EXE" "$test_file" > "$SCRIPT_DIR/temp_output.txt" 2>&1
    local exit_code=$?
    local output=$(cat "$SCRIPT_DIR/temp_output.txt" | grep -v DEBUG)
    
    # Check for syntax error or crash in output
    if echo "$output" | grep -q "Syntax error\|Error at line"; then
        result="FAIL_SYNTAX"
    elif [ "$exit_code" -eq 0 ]; then
        # Exit code 0 = success (pass)
        result="PASS"
    else
        result="FAIL_EXITCODE"
    fi
    
    if [ "$expected" = "pass" ] && [ "$result" = "PASS" ]; then
        echo "✅ PASS"
        ((PASS++))
    elif [ "$expected" = "fail" ] && [ "$result" != "PASS" ]; then
        echo "⏭️  SKIP (expected fail: $result)"
        ((SKIP++))
    else
        echo "❌ FAIL (expected $expected, got $result)"
        ((FAIL++))
    fi
}

echo "=== Implemented Features Tests ==="
for test in "${IMPLEMENTED_TESTS[@]}"; do
    run_test "$TEST_DIR/$test" "pass"
done

echo ""
echo "=== Not Yet Implemented Features Tests (Expected Fail) ==="
for test in "${NOT_IMPLEMENTED_TESTS[@]}"; do
    run_test "$TEST_DIR/$test" "fail"
done

echo ""
echo "=== Summary ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
echo "Skipped (expected): $SKIP"
echo "Total: $((PASS + FAIL + SKIP))"

# Cleanup
rm -f "$SCRIPT_DIR/temp_output.txt"