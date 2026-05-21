#!/bin/bash
GREEN='\033[0;32m'; RED='\033[0;31m'; BLUE='\033[0;34m'; NC='\033[0m'

run_test() {
    local name=$1; local code=$2; local expect=$3
    echo "$code" > temp_test.txt
    local out=$(./compiler < temp_test.txt 2>&1)
    if [ "$expect" == "Valid" ] && echo "$out" | grep -q "(CODE"; then
        echo -e "${GREEN}[V] $name${NC}"
    elif [ "$expect" == "Error" ] && echo "$out" | grep -q "Semantic Error"; then
        echo -e "${GREEN}[V] $name (Error caught)${NC}"
    else
        echo -e "${RED}[X] $name FAILED. Output: $out${NC}"
    fi
}

# --- 1. אופרטורים חשבוניים (+, -, *, /) ---
run_test "Add Int+Int" "proc Main() { var x: int; x = 1 + 2; }" "Valid"
run_test "Add Int+Real" "proc Main() { var r: real; r = 1 + 2.5; }" "Valid"
run_test "Add Bool+Int (Error)" "proc Main() { var x: int; x = true + 1; }" "Error"

# --- 2. אופרטורים לוגיים (&&, ||) ---
run_test "Logic Bool&&Bool" "proc Main() { var b: bool; b = true && false; }" "Valid"
run_test "Logic Int&&Int (Error)" "proc Main() { var b: bool; b = 1 && 0; }" "Error"

# --- 3. השוואות יחס (>, <, >=, <=) ---
run_test "Relational Int < Int" "proc Main() { var b: bool; b = 5 < 10; }" "Valid"
run_test "Relational Real > Int" "proc Main() { var b: bool; b = 5.5 > 1; }" "Valid"
run_test "Relational Bool < Bool (Error)" "proc Main() { var b: bool; b = true < false; }" "Error"

# --- 4. השוואות שוויון (==, !=) ---
run_test "Eq Int == Int" "proc Main() { var b: bool; b = 5 == 5; }" "Valid"
run_test "Eq Ptr == Ptr" "proc Main() { var p1, p2: int*; var b: bool; b = p1 == p2; }" "Valid"
run_test "Eq Int == Real (Error)" "proc Main() { var b: bool; b = 5 == 3.14; }" "Error"

# --- 5. ערך מוחלט (| |) ---
run_test "Length String" "proc Main() { var x: int; var s: string[5]; x = |s|; }" "Valid"
run_test "Length Int (Error)" "proc Main() { var x: int; var s: string[5]; x = |s|; }" "Error"

# --- 6. אופרטור NOT (!) ---
run_test "Not Bool" "proc Main() { var b: bool; b = !true; }" "Valid"
run_test "Not Int (Error)" "proc Main() { var b: bool; b = !10; }" "Error"

rm -f temp_test.txt