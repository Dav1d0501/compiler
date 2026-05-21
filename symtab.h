#ifndef SYMTAB_H
#define SYMTAB_H

typedef enum {
    TYPE_UNKNOWN, 
    TYPE_INT, 
    TYPE_REAL, 
    TYPE_BOOL, 
    TYPE_CHAR, 
    TYPE_STRING,
    TYPE_INT_PTR, 
    TYPE_REAL_PTR, 
    TYPE_CHAR_PTR, 
    TYPE_VOID 
} DataType;

// סוג הסמל: משתנה, פונקציה או פרוצדורה
typedef enum { SYM_VAR, SYM_FUNC, SYM_PROC } SymType;

// רשומה בודדת בטבלת הסמלים
typedef struct Symbol {
    char* name;             // שם המשתנה/פונקציה
    SymType sym_type;       // התפקיד שלו
    DataType data_type;     // סוג הנתון שלו 
    int scope_level;        // עומק הבלוק שבו הוא הוגדר
    
    // שדות רלוונטיים רק לפונקציות/פרוצדורות:
    DataType* arg_types;    
    int num_args;           
    
    struct Symbol* next;    // מצביע לסמל הבא באותו Scope
} Symbol;

// הצהרות על פונקציות העזר
void init_symtab();
void push_scope();
void pop_scope();
Symbol* insert_symbol(char* name, SymType sym_type, DataType data_type);
Symbol* lookup_symbol(char* name);

#endif