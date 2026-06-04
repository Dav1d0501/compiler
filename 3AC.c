#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* מונים גלובליים למשתנים זמניים ותוויות */
int temp_counter = 0;
int label_counter = 1;

char* generate_3ac(node* tree);
void generate_bool_3ac(node* expr, char* true_label, char* false_label);

/* יצירת משתנה זמני חדש */
char* new_temp() {
    char* temp = (char*)malloc(10);
    sprintf(temp, "t%d", temp_counter++);
    return temp;
}

/* יצירת תווית חדשה */
char* new_label() {
    char* label = (char*)malloc(10);
    sprintf(label, "L%d", label_counter++);
    return label;
}

/* בדיקה האם הערך הוא קבוע (Literal) */
int is_literal(const char* token) {
    if (!token) return 0;
    if (token[0] >= '0' && token[0] <= '9') return 1;
    if (token[0] == '-' && token[1] >= '0' && token[1] <= '9') return 1;
    if (token[0] == '\'' || token[0] == '\"') return 1;
    if (strcmp(token, "TRUE") == 0 || strcmp(token, "FALSE") == 0) return 1;
    if (strcmp(token, "NULL") == 0) return 1;
    return 0;
}

/* פונקציית עזר להמרת קבוע למשתנה זמני רק היכן שצריך (השמות ופעולות) */
char* ensure_temp_if_literal(char* val) {
    if (is_literal(val)) {
        char* temp = new_temp();
        printf("\t%s = %s\n", temp, val);
        return temp;
    }
    return val;
}

/* דחיפת פרמטרים מימין לשמאל */
int push_args(node* arg_tree) {
    if (arg_tree == NULL || (arg_tree->token && strcmp(arg_tree->token, "NONE") == 0)) {
        return 0;
    }
    
    if (arg_tree->token && strcmp(arg_tree->token, "") == 0) {
        int count = push_args(arg_tree->right); 
        char* arg_val = generate_3ac(arg_tree->left);
        arg_val = ensure_temp_if_literal(arg_val);
        printf("\tPushParam %s\n", arg_val);
        return count + 1;
    } else {
        char* arg_val = generate_3ac(arg_tree);
        arg_val = ensure_temp_if_literal(arg_val);
        printf("\tPushParam %s\n", arg_val);
        return 1;
    }
}

/* יצירת קוד 3AC עבור ביטויים לוגיים (Short-Circuit Evaluation) */
void generate_bool_3ac(node* expr, char* true_label, char* false_label) {
    if (expr == NULL) return;

    if (strcmp(expr->token, "||") == 0) {
        char* next_label = new_label();
        generate_bool_3ac(expr->left, true_label, next_label);
        printf("%s:\n", next_label);
        generate_bool_3ac(expr->right, true_label, false_label);
        return;
    }

    if (strcmp(expr->token, "&&") == 0) {
        char* next_label = new_label();
        generate_bool_3ac(expr->left, next_label, false_label);
        printf("%s:\n", next_label);
        generate_bool_3ac(expr->right, true_label, false_label);
        return;
    }

    if (strcmp(expr->token, "!") == 0) {
        generate_bool_3ac(expr->left, false_label, true_label);
        return;
    }

    if (strcmp(expr->token, "==") == 0 || strcmp(expr->token, "!=") == 0 ||
        strcmp(expr->token, "<") == 0  || strcmp(expr->token, ">") == 0 ||
        strcmp(expr->token, "<=") == 0 || strcmp(expr->token, ">=") == 0) {
        
        char* left_val = generate_3ac(expr->left);
        char* right_val = generate_3ac(expr->right);
        
        // בתנאי יחסי, קבועים לא מקבלים משתנה זמני לפי הדוגמא בהרצאה!
        printf("\tif %s %s %s Goto %s\n", left_val, expr->token, right_val, true_label);
        printf("\tgoto %s\n", false_label);
        return;
    }

    if (strcmp(expr->token, "TRUE") == 0) {
        printf("\tGoto %s\n", true_label);
        return;
    }
    if (strcmp(expr->token, "FALSE") == 0) {
        printf("\tgoto %s\n", false_label);
        return;
    }

    char* val = generate_3ac(expr);
    val = ensure_temp_if_literal(val);
    printf("\tif %s == 1 Goto %s\n", val, true_label);
    printf("\tgoto %s\n", false_label);
}

/* הפונקציה המרכזית לסיור בעץ וייצור קוד 3AC */
char* generate_3ac(node* tree) {
    if (tree == NULL) return NULL;

    // התעלמות מצמתים ריקים שמשמשים רק לחיבור בעץ
    if (tree->token && strcmp(tree->token, "") == 0) {
        generate_3ac(tree->left);
        generate_3ac(tree->right);
        return NULL;
    }

    // טיפול בבלוק קוד
    if (strcmp(tree->token, "BLOCK") == 0 || strcmp(tree->token, "BODY") == 0) {
        generate_3ac(tree->left);
        return NULL;
    }

    // הגענו לעלה - מחזירים את הטוקן, ומנקים אפסים מיותרים משברים עשרוניים
    if (tree->left == NULL && tree->right == NULL) {
        if (strchr(tree->token, '.') != NULL && (tree->token[0] >= '0' && tree->token[0] <= '9')) {
            char* clean_val = strdup(tree->token);
            int len = strlen(clean_val);
            while (len > 0 && clean_val[len-1] == '0') {
                clean_val[len-1] = '\0';
                len--;
            }
            if (len > 0 && clean_val[len-1] == '.') clean_val[len-1] = '\0';
            return clean_val;
        }
        return tree->token; 
    }

    // הגדרות טיפוסים שלא מייצרות קוד
    if (strcmp(tree->token, "INT") == 0 || strcmp(tree->token, "REAL") == 0 || 
        strcmp(tree->token, "BOOL") == 0 || strcmp(tree->token, "CHAR") == 0 ||
        strcmp(tree->token, "INT*") == 0 || strcmp(tree->token, "REAL*") == 0 || 
        strcmp(tree->token, "CHAR*") == 0) {
        return NULL;
    }

    // אופרטורים אריתמטיים
    if (strcmp(tree->token, "UMINUS") == 0) {
        char* child_val = generate_3ac(tree->left);
        child_val = ensure_temp_if_literal(child_val);
        char* result_temp = new_temp();
        printf("\t%s = - %s\n", result_temp, child_val);
        return result_temp;
    }

    if (strcmp(tree->token, "+") == 0 || strcmp(tree->token, "-") == 0 ||
        strcmp(tree->token, "*") == 0 || strcmp(tree->token, "/") == 0) {
        char* left_val = generate_3ac(tree->left);
        char* right_val = generate_3ac(tree->right);
        
        left_val = ensure_temp_if_literal(left_val);
        right_val = ensure_temp_if_literal(right_val);
        
        char* result_temp = new_temp();
        printf("\t%s = %s %s %s\n", result_temp, left_val, tree->token, right_val);
        return result_temp;
    }

    // משפטי השמה
    if (strcmp(tree->token, "=") == 0) {
        char* right_val = generate_3ac(tree->right);
        right_val = ensure_temp_if_literal(right_val);
        
        if (strcmp(tree->left->token, "^") == 0) {
            char* ptr_val = generate_3ac(tree->left->left);
            printf("\t*%s = %s\n", ptr_val, right_val);
        } 
        else if (strcmp(tree->left->token, "[]") == 0) {
            char* arr_name = tree->left->left->token;
            char* idx_val = generate_3ac(tree->left->right);
            idx_val = ensure_temp_if_literal(idx_val);
            printf("\t%s[%s] = %s\n", arr_name, idx_val, right_val);
        } 
        else {
            char* left_var = tree->left->token;
            printf("\t%s = %s\n", left_var, right_val);
        }
        return NULL;
    }

    // מצביעים ומערכים
    if (strcmp(tree->token, "&") == 0) {
        char* child_val = generate_3ac(tree->left);
        char* temp = new_temp();
        printf("\t%s = & %s\n", temp, child_val);
        return temp;
    }
    if (strcmp(tree->token, "^") == 0) {
        char* ptr_val = generate_3ac(tree->left);
        char* temp = new_temp();
        printf("\t%s = *%s\n", temp, ptr_val); 
        return temp;
    }
    if (strcmp(tree->token, "[]") == 0) {
        char* arr_name = tree->left->token;
        char* idx_val = generate_3ac(tree->right);
        idx_val = ensure_temp_if_literal(idx_val);
        char* temp = new_temp();
        printf("\t%s = %s[%s]\n", temp, arr_name, idx_val);
        return temp;
    }
    if (strcmp(tree->token, "|length|") == 0) {
        char* str_val = generate_3ac(tree->left);
        char* temp = new_temp();
        printf("\t%s = length %s\n", temp, str_val);
        return temp;
    }

    // הגדרת פונקציות ופרוצדורות
    if (strcmp(tree->token, "FUNC") == 0) {
        char* func_name = tree->left->token;
        printf("%s:\n", func_name);
        printf("\tBeginFunc 24\n");
        node* curr = tree->right; 
        if (curr && curr->right) curr = curr->right;
        if (curr && curr->right) curr = curr->right;
        generate_3ac(curr); 
        printf("\tEndFunc\n");
        return NULL;
    }

    if (strcmp(tree->token, "PROC") == 0) {
        char* func_name = tree->left->token;
        if (strcmp(func_name, "Main") == 0 || strcmp(func_name, "main") == 0) {
            printf("main:\n"); 
        } else {
            printf("%s:\n", func_name);
        }
        printf("\tBeginFunc 28\n");
        node* curr = tree->right;
        if (curr && curr->right) curr = curr->right;
        generate_3ac(curr);
        printf("\tEndFunc\n");
        return NULL;
    }

    if (strcmp(tree->token, "RET") == 0) {
        if (tree->left == NULL || strcmp(tree->left->token, "NONE") == 0) {
            printf("\tReturn\n");
        } else {
            char* ret_val = generate_3ac(tree->left);
            ret_val = ensure_temp_if_literal(ret_val);
            printf("\tReturn %s\n", ret_val);
        }
        return NULL;
    }

    // קריאה לפונקציות (LCall)
    if (strcmp(tree->token, "CALL") == 0) {
        char* func_name = tree->left->token;
        int num_args = push_args(tree->right);
        char* result_temp = new_temp();
        printf("\t%s = LCall %s\n", result_temp, func_name);
        if (num_args > 0) {
            printf("\tPopParams %d\n", num_args * 8);
        }
        return result_temp;
    }

    // מבני בקרה
    if (strcmp(tree->token, "IF") == 0 || strcmp(tree->token, "IF-ELSE") == 0) {
        char* true_label = new_label();
        char* false_label = new_label();
        
        if (strcmp(tree->token, "IF-ELSE") == 0) {
            char* end_label = new_label();
            generate_bool_3ac(tree->left, true_label, false_label);
            printf("%s:\n", true_label);
            generate_3ac(tree->right->left); 
            printf("\tGoto %s\n", end_label);
            printf("%s:\n", false_label);
            generate_3ac(tree->right->right); 
            printf("%s:\n", end_label);
        } else {
            generate_bool_3ac(tree->left, true_label, false_label);
            printf("%s:\n", true_label);
            generate_3ac(tree->right); 
            printf("%s:\n", false_label);
        }
        return NULL;
    }

    if (strcmp(tree->token, "WHILE") == 0) {
        char* start_label = new_label();
        char* true_label = new_label();
        char* false_label = new_label();
        
        printf("%s:\n", start_label);
        generate_bool_3ac(tree->left, true_label, false_label);
        printf("%s:\n", true_label);
        generate_3ac(tree->right);
        printf("\tGoto %s\n", start_label);
        printf("%s:\n", false_label);
        return NULL;
    }

    if (strcmp(tree->token, "FOR") == 0) {
        generate_3ac(tree->left); 
        char* start_label = new_label();
        char* true_label = new_label();
        char* false_label = new_label();
        
        printf("%s:\n", start_label);
        generate_bool_3ac(tree->right->left, true_label, false_label); 
        
        printf("%s:\n", true_label);
        generate_3ac(tree->right->right->right); 
        generate_3ac(tree->right->right->left);  
        printf("\tGoto %s\n", start_label);
        
        printf("%s:\n", false_label);
        return NULL;
    }

    // ביטויים בוליאניים כערך מחוץ למשפט בקרה
    if (strcmp(tree->token, "==") == 0 || strcmp(tree->token, "!=") == 0 ||
        strcmp(tree->token, "<") == 0  || strcmp(tree->token, ">") == 0 ||
        strcmp(tree->token, "<=") == 0 || strcmp(tree->token, ">=") == 0 ||
        strcmp(tree->token, "&&") == 0 || strcmp(tree->token, "||") == 0 ||
        strcmp(tree->token, "!") == 0) {
        
        char* temp = new_temp();
        char* true_label = new_label();
        char* false_label = new_label();
        char* end_label = new_label();
        
        generate_bool_3ac(tree, true_label, false_label);
        printf("%s:\n", true_label);
        printf("\t%s = 1\n", temp);
        printf("\tGoto %s\n", end_label);
        printf("%s:\n", false_label);
        printf("\t%s = 0\n", temp);
        printf("%s:\n", end_label);
        return temp;
    }

    generate_3ac(tree->left);
    generate_3ac(tree->right);
    return NULL;
}