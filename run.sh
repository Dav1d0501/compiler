#!/bin/bash

# Find this shell script's directory - DO NOT DELETE
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Compile the lex and yacc files - Change if needed
lex temp.l
yacc -d temp.y
gcc y.tab.c -o compiler -lfl
./compiler < input4.txt > output.txt