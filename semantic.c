#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ast.h"
#include "symtab.h"
#include "semantic.h"

int has_main = 0;
DataType current_func_ret_type = TYPE_VOID; // למעקב אחרי חוק 9

//  פונקציות עזר לזיהוי טוקנים 

int is_type_token(const char* token) {
    if(!token) return 0;
    if(strcmp(token, "INT") == 0 || strcmp(token, "REAL") == 0 ||
       strcmp(token, "BOOL") == 0 || strcmp(token, "CHAR") == 0 ||
       strcmp(token, "INT*") == 0 || strcmp(token, "REAL*") == 0 ||
       strcmp(token, "CHAR*") == 0) return 1;
    if(strncmp(token, "STRING[", 7) == 0) return 1;
    return 0;
}

DataType map_string_to_type(char* type_str) {
    if(!type_str) return TYPE_UNKNOWN;
    if(strcmp(type_str, "INT") == 0) return TYPE_INT;
    if(strcmp(type_str, "REAL") == 0) return TYPE_REAL;
    if(strcmp(type_str, "BOOL") == 0) return TYPE_BOOL;
    if(strcmp(type_str, "CHAR") == 0) return TYPE_CHAR;
    if(strncmp(type_str, "STRING[", 7) == 0) return TYPE_STRING;
    if(strcmp(type_str, "INT*") == 0) return TYPE_INT_PTR;
    if(strcmp(type_str, "REAL*") == 0) return TYPE_REAL_PTR;
    if(strcmp(type_str, "CHAR*") == 0) return TYPE_CHAR_PTR;
    return TYPE_UNKNOWN;
}

int is_identifier(const char* token) {
    if (!token) return 0;
    char c = token[0];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) return 0;
    
    // מניעת בלבול עם מילות מפתח פנימיות של העץ
    const char* keywords[] = {"INT", "REAL", "BOOL", "CHAR", "STRING", "INT*", "REAL*", "CHAR*", 
                              "NONE", "TRUE", "FALSE", "NULL", "BLOCK", "BODY", "ARGS", "RET", 
                              "FUNC", "PROC", "IF", "IF-ELSE", "WHILE", "FOR", "CALL", "UMINUS"};
    for(int i=0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        if (strcmp(token, keywords[i]) == 0) return 0;
    }
    if (strncmp(token, "STRING[", 7) == 0) return 0;
    return 1;
}

DataType infer_leaf_type(const char* token) {
    if (!token) return TYPE_UNKNOWN;
    if (strcmp(token, "TRUE") == 0 || strcmp(token, "FALSE") == 0) return TYPE_BOOL;
    if (strcmp(token, "NULL") == 0) return TYPE_VOID;
    if (token[0] == '\'') return TYPE_CHAR;
    if (token[0] == '"') return TYPE_STRING;
    if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
        if (strchr(token, '.')) return TYPE_REAL;
        return TYPE_INT;
    }
    return TYPE_UNKNOWN;
}

//  ספירה ובדיקת ארגומנטים (חוקים 7, 8) 

int count_args_in_list(node* args) {
    if (!args) return 0;
    if (args->token && strcmp(args->token, "") == 0) {
        return count_args_in_list(args->left) + count_args_in_list(args->right);
    }
    if (is_identifier(args->token)) return 1;
    return 0;
}

int count_formal_args(node* arg_list) {
    if (!arg_list) return 0;
    if (arg_list->token && strcmp(arg_list->token, "") == 0) {
        return count_formal_args(arg_list->left) + count_formal_args(arg_list->right);
    }
    if (is_type_token(arg_list->token)) {
        return count_args_in_list(arg_list->left);
    }
    return 0;
}

void collect_formal_args(node* arg_list, DataType* type_array, int* index) {
    if (!arg_list) return;
    if (arg_list->token && strcmp(arg_list->token, "") == 0) {
        collect_formal_args(arg_list->left, type_array, index);
        collect_formal_args(arg_list->right, type_array, index);
    } else if (is_type_token(arg_list->token)) {
        DataType dt = map_string_to_type(arg_list->token);
        int count = count_args_in_list(arg_list->left);
        for(int i = 0; i < count; i++) {
            if (type_array) type_array[*index] = dt;
            (*index)++;
        }
    }
}

int count_actual_args(node* expr_list) {
    if (!expr_list) return 0;
    if (expr_list->token && strcmp(expr_list->token, "NONE") == 0) return 0;
    if (expr_list->token && strcmp(expr_list->token, "") == 0) {
        return count_actual_args(expr_list->left) + count_actual_args(expr_list->right);
    }
    return 1;
}

void check_actual_args(node* expr_list, Symbol* sym, int* index, char* func_name) {
    if (!expr_list) return;
    if (expr_list->token && strcmp(expr_list->token, "NONE") == 0) return;
    
    if (expr_list->token && strcmp(expr_list->token, "") == 0) {
        check_actual_args(expr_list->left, sym, index, func_name);
        check_actual_args(expr_list->right, sym, index, func_name);
    } else {
        if (*index >= sym->num_args) return;
        
        DataType expected = sym->arg_types[*index];
        DataType actual = expr_list->eval_type;
        
        int is_valid = 0;
        if (expected == actual) is_valid = 1;
        if (actual == TYPE_VOID && (expected == TYPE_INT_PTR || expected == TYPE_REAL_PTR || expected == TYPE_CHAR_PTR)) is_valid = 1;
        
        if (!is_valid) {
            printf("Semantic Error: Argument %d in call to '%s' type mismatch .\n", (*index)+1, func_name);
            exit(1);
        }
        (*index)++;
    }
}

// רישום משתנים לטבלה (חוק 4)
void insert_var_list(node* n, DataType dt) {
    if (!n) return;
    if (n->token && strcmp(n->token, "") == 0) {
        insert_var_list(n->left, dt);
        insert_var_list(n->right, dt);
    } else if (n->token) {
        if (!insert_symbol(n->token, SYM_VAR, dt)) {
            printf("Semantic Error: Variable '%s' already defined in this scope .\n", n->token);
            exit(1);
        }
    }
}

//  סריקת העץ 

void traverse(node* n) {
    if(!n) return;
    
    int new_scope_opened = 0;

    //  טיפול ייעודי בקריאות לפונקציות כדי למנוע התנגשויות (חוקים 5, 7, 8) 
    if (n->token && strcmp(n->token, "CALL") == 0) {
        char* func_name = n->left->token;
        Symbol* sym = lookup_symbol(func_name);
        
        if (!sym || (sym->sym_type != SYM_FUNC && sym->sym_type != SYM_PROC)) {
            printf("Semantic Error: Function/Procedure '%s' is not a defined function .\n", func_name);
            exit(1);
        }
        
        traverse(n->right); // סורקים רק את הארגומנטים, מדלגים על שם הפונקציה
        
        int actual_args = count_actual_args(n->right);
        if (actual_args != sym->num_args) {
            printf("Semantic Error: '%s' expects %d args, got %d .\n", func_name, sym->num_args, actual_args);
            exit(1);
        }
        
        int idx = 0;
        check_actual_args(n->right, sym, &idx, func_name);
        n->eval_type = sym->data_type;
        return; // עוצרים כאן כדי שלא יתייחס לשם הפונקציה כמשתנה חסר
    }

    //  פתיחת Scope ורישום פונקציות 
    if (n->token && (strcmp(n->token, "PROC") == 0 || strcmp(n->token, "FUNC") == 0)) {
        char* name = n->left->token;
        SymType stype = (strcmp(n->token, "PROC") == 0) ? SYM_PROC : SYM_FUNC;
        DataType ret_type = TYPE_VOID;
        
        if (stype == SYM_FUNC) {
            node* ret_node = n->right->right->left;
            if (ret_node && strcmp(ret_node->token, "RET") == 0 && ret_node->left) {
                ret_type = map_string_to_type(ret_node->left->token);
                if (ret_type == TYPE_STRING) {
                    printf("Semantic Error: Function '%s' cannot return a string .\n", name);
                    exit(1);
                }
            }
        }
        
        if (strcmp(name, "Main") == 0) {
            has_main = 1;
            node* args_node = n->right->left; 
            if (args_node && args_node->left && strcmp(args_node->left->token, "NONE") != 0) {
                printf("Semantic Error: Main procedure cannot take arguments .\n");
                exit(1);
            }
        }
        
        Symbol* sym = insert_symbol(name, stype, ret_type);
        if(!sym) {
            printf("Semantic Error: Function/Procedure '%s' already defined .\n", name);
            exit(1);
        }
        
        node* args_node = n->right->left; 
        int total_args = count_formal_args(args_node->left);
        sym->num_args = total_args;
        if (total_args > 0) {
            sym->arg_types = (DataType*)malloc(sizeof(DataType) * total_args);
            int temp_idx = 0;
            collect_formal_args(args_node->left, sym->arg_types, &temp_idx);
        }
        current_func_ret_type = ret_type;
    }

    if(n->token && (strcmp(n->token, "BLOCK") == 0 || strcmp(n->token, "FUNC") == 0 || strcmp(n->token, "PROC") == 0)) {
        push_scope();
        new_scope_opened = 1;
    }

    // חוק 4: רישום משתנים
    if (n->token && is_type_token(n->token) && n->left) {
        DataType dt = map_string_to_type(n->token);
        insert_var_list(n->left, dt);
    }
    
    // חוק 6: שימוש במשתנה לפני הגדרה
    if (!n->left && !n->right && is_identifier(n->token)) {
        Symbol* sym = lookup_symbol(n->token);
        if (!sym) {
            printf("Semantic Error: Variable '%s' not defined before use .\n", n->token);
            exit(1);
        }
        n->eval_type = sym->data_type; 
    }

    traverse(n->left);
    traverse(n->right);
    
    //  בתר-סריקה: חישוב טיפוסים ואכיפת חוקים מתמטיים 

    if (n->token) {
        if (!n->left && !n->right && !is_identifier(n->token)) {
            n->eval_type = infer_leaf_type(n->token);
        }
        
        // חוק 9: בדיקת טיפוס החזרה מול החתימה 
        if (strcmp(n->token, "RET") == 0) {
            if (!(n->left && is_type_token(n->left->token))) {
                DataType actual_ret = (n->left && strcmp(n->left->token, "NONE")!=0) ? n->left->eval_type : TYPE_VOID;
                if (actual_ret != current_func_ret_type) {
                    printf("Semantic Error: Return type mismatch in function .\n");
                    exit(1);
                }
            }
        }

        // חוקים 11-12: תנאי לולאות ו-IF
        if (strcmp(n->token, "IF") == 0 || strcmp(n->token, "IF-ELSE") == 0) {
            if (n->left->eval_type != TYPE_BOOL) {
                printf("Semantic Error: 'if' condition must be boolean .\n");
                exit(1);
            }
        }
        if (strcmp(n->token, "WHILE") == 0) {
            if (n->left->eval_type != TYPE_BOOL) {
                printf("Semantic Error: 'while' condition must be boolean .\n");
                exit(1);
            }
        }
        if (strcmp(n->token, "FOR") == 0) {
            node* expr = n->right->left; 
            if (expr && expr->eval_type != TYPE_BOOL) {
                printf("Semantic Error: 'for' condition must be boolean .\n");
                exit(1);
            }
        }
        // חוק 15: השמה
        if (strcmp(n->token, "=") == 0) {
            DataType left_t = n->left->eval_type;
            DataType right_t = n->right->eval_type;
            
            int is_valid = 0;
            if (left_t == right_t) is_valid = 1;
            if (right_t == TYPE_VOID && (left_t == TYPE_INT_PTR || left_t == TYPE_REAL_PTR || left_t == TYPE_CHAR_PTR)) is_valid = 1;
            if(left_t == TYPE_REAL && right_t == TYPE_INT) is_valid = 1; 
            if (!is_valid) {
                printf("Semantic Error: Type mismatch in assignment .\n");
                exit(1);
            }
            n->eval_type = left_t;
        }

        //  חוק 16-19: אופרטורים וביטויים 

        if (strcmp(n->token, "UMINUS") == 0) {
            DataType child_t = n->left->eval_type;
            if (child_t != TYPE_INT && child_t != TYPE_REAL) {
                printf("Semantic Error: Unary minus requires a numeric operand .\n");
                exit(1);
            }
            n->eval_type = child_t;
        }
        
        if (strcmp(n->token, "+") == 0 || strcmp(n->token, "-") == 0) {
            DataType lt = n->left->eval_type;
            DataType rt = n->right->eval_type;
            
            if ((lt == TYPE_INT_PTR || lt == TYPE_REAL_PTR || lt == TYPE_CHAR_PTR) && rt == TYPE_INT) {
                n->eval_type = lt;
            } 
            else if (lt == TYPE_INT && (rt == TYPE_INT_PTR || rt == TYPE_REAL_PTR || rt == TYPE_CHAR_PTR) && strcmp(n->token, "+") == 0) {
                n->eval_type = rt;
            }
            else if ((lt == TYPE_INT || lt == TYPE_REAL) && (rt == TYPE_INT || rt == TYPE_REAL)) {
                n->eval_type = (lt == TYPE_INT && rt == TYPE_INT) ? TYPE_INT : TYPE_REAL;
            } else {
                printf("Semantic Error: Invalid types for operator '%s' .\n", n->token);
                exit(1);
            }
        }
        
        if (strcmp(n->token, "*") == 0 || strcmp(n->token, "/") == 0) {
            DataType lt = n->left->eval_type;
            DataType rt = n->right->eval_type;
            if ((lt == TYPE_INT || lt == TYPE_REAL) && (rt == TYPE_INT || rt == TYPE_REAL)) {
                n->eval_type = (lt == TYPE_INT && rt == TYPE_INT) ? TYPE_INT : TYPE_REAL;
            } else {
                printf("Semantic Error: Invalid types for operator '%s' .\n", n->token);
                exit(1);
            }
        }
        
        if (strcmp(n->token, "&&") == 0 || strcmp(n->token, "||") == 0) {
            if (n->left->eval_type == TYPE_BOOL && n->right->eval_type == TYPE_BOOL) {
                n->eval_type = TYPE_BOOL;
            } else {
                printf("Semantic Error: Operators &&, || require boolean operands .\n");
                exit(1);
            }
        }
        
        if (strcmp(n->token, "<") == 0 || strcmp(n->token, ">") == 0 || strcmp(n->token, "<=") == 0 || strcmp(n->token, ">=") == 0) {
            DataType lt = n->left->eval_type;
            DataType rt = n->right->eval_type;
            if ((lt == TYPE_INT || lt == TYPE_REAL) && (rt == TYPE_INT || rt == TYPE_REAL)) {
                n->eval_type = TYPE_BOOL;
            } else {
                printf("Semantic Error: Relational operators require numeric operands .\n");
                exit(1);
            }
        }
        
        if (strcmp(n->token, "==") == 0 || strcmp(n->token, "!=") == 0) {
            DataType lt = n->left->eval_type;
            DataType rt = n->right->eval_type;
            int is_valid = 0;

            if (lt == rt) {
                // בדיקה שסוגי הנתונים השווים הם רק אלו המותרים בחוק 16
                if (lt == TYPE_INT || lt == TYPE_BOOL || lt == TYPE_REAL || lt == TYPE_CHAR ||
                    lt == TYPE_INT_PTR || lt == TYPE_REAL_PTR || lt == TYPE_CHAR_PTR) {
                    is_valid = 1;
                }
            } else if ((lt == TYPE_VOID && (rt == TYPE_INT_PTR || rt == TYPE_REAL_PTR || rt == TYPE_CHAR_PTR)) || 
                       (rt == TYPE_VOID && (lt == TYPE_INT_PTR || lt == TYPE_REAL_PTR || lt == TYPE_CHAR_PTR))) {
                is_valid = 1; // מאפשר השוואת NULL עם מצביע
            }

            if (is_valid) {
                n->eval_type = TYPE_BOOL;
            } else {
                printf("Semantic Error: Operands of == or != must match in type .\n");
                exit(1);
            }
        }
        
        if (strcmp(n->token, "!") == 0) {
            if (n->left->eval_type != TYPE_BOOL) {
                printf("Semantic Error: Operator '!' requires a boolean operand .\n");
                exit(1);
            }
            n->eval_type = TYPE_BOOL;
        }
        
        if (strcmp(n->token, "|length|") == 0) {
            if (n->left->eval_type != TYPE_STRING) {
                printf("Semantic Error: Length operator | | requires a string operand .\n");
                exit(1);
            }
            n->eval_type = TYPE_INT;
        }

        if (strcmp(n->token, "&") == 0) {
            DataType child_t = n->left->eval_type;
            if (child_t != TYPE_INT && child_t != TYPE_REAL && child_t != TYPE_CHAR && child_t != TYPE_STRING) {
                printf("Semantic Error: Address operator '&' applied to invalid type .\n");
                exit(1);
            }
            if (child_t == TYPE_INT) n->eval_type = TYPE_INT_PTR;
            else if (child_t == TYPE_REAL) n->eval_type = TYPE_REAL_PTR;
            else if (child_t == TYPE_CHAR || child_t == TYPE_STRING) n->eval_type = TYPE_CHAR_PTR;
        }

        if (strcmp(n->token, "^") == 0) {
            DataType child_t = n->left->eval_type;
            if (child_t != TYPE_INT_PTR && child_t != TYPE_REAL_PTR && child_t != TYPE_CHAR_PTR) {
                printf("Semantic Error: Deref operator '^' can only be applied to pointers .\n");
                exit(1);
            }
            if (child_t == TYPE_INT_PTR) n->eval_type = TYPE_INT;
            else if (child_t == TYPE_REAL_PTR) n->eval_type = TYPE_REAL;
            else if (child_t == TYPE_CHAR_PTR) n->eval_type = TYPE_CHAR;
        }
    }

    if(new_scope_opened) {
        pop_scope();
    }
}

void check_semantics(node* root) {
    init_symtab();
    has_main = 0;
    
    traverse(root);
    
    if (!has_main) {
        printf("Semantic Error: Missing 'Main' procedure .\n");
        exit(1);
    }
}