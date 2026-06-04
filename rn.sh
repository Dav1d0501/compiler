#!/bin/bash

# Find this shell script's directory - DO NOT DELETE
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building the compiler..."

# Compile the lex and yacc files
lex temp.l
yacc -d temp.y

# Compile all C files together, including the 3AC generator
gcc y.tab.c symtab.c semantic.c 3AC.c -o compiler -lfl

# Check if the compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Running the compiler..."
    
    # Run the compiler with input.txt and redirect to output.txt
    ./compiler < input.txt > output.txt
    
    echo "Done! Check output.txt for the generated 3AC."
else
    echo "Compilation failed. Please fix the errors above and try again."
fi