#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

#define MAX_SCOPES 100

// מחסנית של טבלאות, כל תא הוא בלוק
Symbol* scope_stack[MAX_SCOPES];
int current_scope = 0;

void init_symtab() {
    current_scope = 0;
    for(int i = 0; i < MAX_SCOPES; i++) {
        scope_stack[i] = NULL;
    }
}

// כניסה לבלוק חדש
void push_scope() {
    if(current_scope < MAX_SCOPES - 1) {
        current_scope++;
        scope_stack[current_scope] = NULL; 
    } else { 
        printf("שגיאה: הגענו לעומק בלוקים מקסימלי.\n"); 
        exit(1); 
    }
}

// יציאה מבלוק ושחרור זיכרון
void pop_scope() {
    if(current_scope > 0) {
        Symbol* curr = scope_stack[current_scope];
        while(curr) {
            Symbol* temp = curr;
            curr = curr->next;
            free(temp->name);
            if(temp->arg_types) free(temp->arg_types);
            free(temp);
        }
        scope_stack[current_scope] = NULL;
        current_scope--;
    }
}

// הוספת משתנה או פונקציה לטבלה
Symbol* insert_symbol(char* name, SymType sym_type, DataType data_type) {
    Symbol* curr = scope_stack[current_scope];
    while(curr) {
        if(strcmp(curr->name, name) == 0) return NULL; 
        curr = curr->next;
    }
    
    Symbol* new_sym = (Symbol*)malloc(sizeof(Symbol));
    new_sym->name = strdup(name);
    new_sym->sym_type = sym_type;
    new_sym->data_type = data_type;
    new_sym->scope_level = current_scope;
    new_sym->arg_types = NULL;
    new_sym->num_args = 0;
    
    new_sym->next = scope_stack[current_scope];
    scope_stack[current_scope] = new_sym;
    
    return new_sym;
}

// חיפוש משתנה או פונקציה מהבלוק הנוכחי והלאה
Symbol* lookup_symbol(char* name) {
    for(int i = current_scope; i >= 0; i--) {
        Symbol* curr = scope_stack[i];
        while(curr) {
            if(strcmp(curr->name, name) == 0) return curr;
            curr = curr->next;
        }
    }
    return NULL;
}