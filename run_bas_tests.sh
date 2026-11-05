#!/bin/bash
# GW-BASIC test runner - categorizes tests by feature status with output validation

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/tests/bas"
GWBASIC_EXE="$SCRIPT_DIR/gwbasic.exe"

PASS=0
FAIL=0
SKIP=0

# Tests that should PASS (implemented features) with expected output files
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
    "test_string.bas"
    "test_string_funcs.bas"
    "test_string_simple.bas"
    "test_string_var.bas"
    "test_len.bas"
    "test_len2.bas"
    "test_left.bas"
    "test_left_assign.bas"
    "test_left_direct.bas"
    "test_left_direct2.bas"
    "test_left_only.bas"
    "test_left_print.bas"
    "test_left_simple.bas"
    "test_while_wend.bas"
    "test_while_debug.bas"
    "test_file_seq.bas"
    "test_line_input.bas"
    "test_kill_temp.bas"
    "test_minimal.bas"
    "test_read_data.bas"
    "test_file_rand.bas"
    "test_put_get.bas"
    "test_string_arrays.bas"
    "test_using_simple.bas"
    "test_using_very_simple.bas"
    "test_print_using.bas"
    "test_error.bas"
    "test_debug.bas"
    "test_div.bas"
    "test_err2.bas"
    "test_error_stmt.bas"
    "test_onerr_simple.bas"
    "test_resume_next.bas"
    "test_resume_line.bas"
    "test_def_fn.bas"
    "test_renum.bas"
    "test_delete.bas"
    "test_delete_exec.bas"
    "test_gwdata.bas"
    "test_gwram.bas"
    "test_on_goto.bas"
    "test_if_else.bas"
    "test_graphics.bas"
    "test_gfx_v141.bas"
    "test_gfx_circle.bas"
    "test_gfx_getput.bas"
    "test_for_edges.bas"
    "test_for_lastnext.bas"
    "test_if_lineno.bas"
    "test_open_classic.bas"
    "test_open_varspec.bas"
    "test_gfx_screen0.bas"
    "test_gfx_screen0_resume.bas"
    "test_m4_funcs.bas"
    "test_using.bas"
    "test_onerr_off.bas"
)

# Tests for NOT YET IMPLEMENTED features (expected to fail)
NOT_IMPLEMENTED_TESTS=(
)

# Compare output with expected file
# Returns 0 if match, 1 if mismatch, 2 if no expected file
compare_output() {
    local test_name=$1
    local actual_file="$SCRIPT_DIR/temp_output.txt"
    local expected_file="$TEST_DIR/${test_name}.expected"
    
    if [ ! -f "$expected_file" ]; then
        return 2  # No expected file
    fi
    
    # Normalize line endings for comparison: convert CRLF -> LF, then any remaining CR -> LF
    local actual_norm=$(cat "$actual_file" | sed 's/\r\n/\n/g' | sed 's/\r/\n/g')
    local expected_norm=$(cat "$expected_file" | sed 's/\r\n/\n/g' | sed 's/\r/\n/g')
    
    if [ "$actual_norm" = "$expected_norm" ]; then
        return 0  # Match
    else
        echo "  Expected:" >&2
        echo "$expected_norm" | sed 's/^/    /' >&2
        echo "  Actual:" >&2
        echo "$actual_norm" | sed 's/^/    /' >&2
        return 1  # Mismatch
    fi
}

# Run a test and check result with output validation
run_test() {
    local test_file=$1
    local expected=$2  # "pass", "fail" or "termerror" (expected terminating error)
    local name=$(basename "$test_file" .bas)
    
    echo -n "Testing $name... "
    
    # Run gwbasic.exe from SCRIPT_DIR with relative test path
    cd "$SCRIPT_DIR"
    local rel_test_file="${test_file#$SCRIPT_DIR/}"
    ./gwbasic.exe "$rel_test_file" > "$SCRIPT_DIR/temp_output.txt" 2>&1
    local exit_code=$?
    local output=$(cat "$SCRIPT_DIR/temp_output.txt" | grep -v DEBUG)
    
    # Check for syntax error or crash in output
    if echo "$output" | grep -q "Syntax error\|Error at line"; then
        result="FAIL_SYNTAX"
    elif [ "$exit_code" -eq 0 ]; then
        result="PASS"
    else
        result="FAIL_EXITCODE"
    fi
    
    # For implemented tests, also validate output content
    # (v130: tests whose .expected includes a terminating error line — e.g.
    # ON ERROR GOTO 0 — validate by CONTENT when syntax/exit-code fail fires.)
    if [ "$expected" = "pass" ] && [ "$result" = "PASS" ]; then
        compare_output "$name"
        local cmp_result=$?
        if [ $cmp_result -eq 1 ]; then
            echo "❌ FAIL (output mismatch)"
            ((FAIL++))
            return
        elif [ $cmp_result -eq 2 ]; then
            echo "⚠️  PASS (no expected output file - output not validated)"
            ((PASS++))
            return
        fi
        # cmp_result = 0 (match)
        echo "✅ PASS"
        ((PASS++))
    elif [ "$expected" = "termerror" ] && [ "$result" = "FAIL_SYNTAX" ]; then
        # Explicitly marked terminating-error tests (v130 LOW-3): rc=127 +
        # "Error at line" is the EXPECTED behavior if .expected matches
        # exactly (output is the contract). Only applies to tests marked
        # "termerror" — normal tests still fail on syntax errors.
        if [ -f "$TEST_DIR/$name.expected" ]; then
            compare_output "$name"
            local cmp_result=$?
            if [ $cmp_result -eq 0 ]; then
                echo "✅ PASS (expected terminating error, output match)"
                ((PASS++))
                return
            fi
        fi
        echo "❌ FAIL ($result)"
        ((FAIL++))
    elif [ "$expected" = "fail" ] && [ "$result" != "PASS" ]; then
        echo "⏭️  SKIP (expected fail: $result)"
        ((SKIP++))
    else
        echo "❌ FAIL (expected $expected, got $result)"
        ((FAIL++))
    fi
}

# Tests that terminate with a legit GW-BASIC error (ON ERROR GOTO 0 etc.)
TERMERROR_TESTS=(
    "test_onerr_off.bas"
)

is_termerror() {
    local t=$1
    for te in "${TERMERROR_TESTS[@]}"; do
        if [ "$t" = "$te" ]; then return 0; fi
    done
    return 1
}

echo "=== Implemented Features Tests ==="
for test in "${IMPLEMENTED_TESTS[@]}"; do
    if is_termerror "$test"; then
        run_test "$TEST_DIR/$test" "termerror"
    else
        run_test "$TEST_DIR/$test" "pass"
    fi
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