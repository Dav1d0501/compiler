%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"

int yylex();
int yyerror(const char *s);
node* root = NULL;

extern int line_number;
extern char* yytext;
%}

%define parse.error verbose

/*  הגדרות טיפוסים וטוקנים */
%union {
    int ival;
    double rval;
    char cval;
    char* sval;
    struct node* nd;
}

%token <ival> LIT_INT
%token <rval> LIT_REAL
%token <cval> LIT_CHAR
%token <sval> LIT_STRING IDENTIFIER
%token BOOL CHAR INT REAL TSTRING INT_PTR CHAR_PTR REAL_PTR
%token IF ELSE WHILE FOR VAR FUNC PROC RETURN TOK_NULL TOK_TRUE TOK_FALSE
%token AND OR EQ NEQ GE LE GT LT NOT
%token PLUS MINUS MULT DIV ASSIGN ADDRESS DEREF LENGTH

%type <nd> program funcs func proc arg_list args type type_literals
%type <nd> var_definition gen_stmts stmts stmt if_stmt while_stmt
%type <nd> for_stmt assign_stmt expr inits updates expr_list

/*  סדר קדימות אופרטורים */
%right ASSIGN 
%left OR
%left AND
%left EQ NEQ
%left LT LE GT GE
%left PLUS MINUS
%left MULT DIV
%right NOT DEREF ADDRESS LENGTH UMINUS UPLUS

%%

/*  חוקי המבנה הראשי  */
program: funcs { root = $1; };

funcs: func funcs { $$ = mknode("", $1, $2); }
     | proc funcs { $$ = mknode("", $1, $2); }
     | func       { $$ = $1; }
     | proc       { $$ = $1; }
     ;

func: FUNC IDENTIFIER '(' arg_list ')' RETURN type '{' gen_stmts '}' { 
        node* id = mknode($2, NULL, NULL);
        node* args = mknode("ARGS", $4, NULL);
        node* ret = mknode("RET", $7, NULL);
        node* body = mknode("BODY", $9, NULL);
        $$ = mknode("FUNC", id, mknode("", args, mknode("", ret, body))); 
    };

proc: PROC IDENTIFIER '(' arg_list ')' '{' gen_stmts '}' { 
        node* id = mknode($2, NULL, NULL);
        node* args = mknode("ARGS", $4, NULL);
        node* body = mknode("BODY", $7, NULL);
        $$ = mknode("PROC", id, mknode("", args, body)); 
    };

/*  פרמטרים וטיפוסים  */
arg_list: args ':' type ';' arg_list { 
            node* t = mknode($3->token, $1, NULL);
            $$ = mknode("", t, $5); 
        }
        | args ':' type { 
            $$ = mknode($3->token, $1, NULL);
        }
        | { $$ = mknode("NONE", NULL, NULL); }
        ;

args: IDENTIFIER ',' args { $$ = mknode("", mknode($1, NULL, NULL), $3); }
    | IDENTIFIER          { $$ = mknode($1, NULL, NULL); }
    ;

type: INT {$$=mknode("INT",NULL,NULL);} | REAL {$$=mknode("REAL",NULL,NULL);} 
    | BOOL {$$=mknode("BOOL",NULL,NULL);} | CHAR {$$=mknode("CHAR",NULL,NULL);}
    | INT_PTR {$$=mknode("INT*",NULL,NULL);} | CHAR_PTR {$$=mknode("CHAR*",NULL,NULL);} 
    | REAL_PTR {$$=mknode("REAL*",NULL,NULL);};

type_literals: LIT_INT {char b[32]; sprintf(b,"%d",$1); $$=mknode(b,NULL,NULL);}
             | LIT_REAL {char b[32]; sprintf(b,"%f",$1); $$=mknode(b,NULL,NULL);}
             | LIT_STRING {$$=mknode($1,NULL,NULL);}
             | LIT_CHAR {char b[4]; sprintf(b,"'%c'",$1); $$=mknode(b,NULL,NULL);}
             | TOK_TRUE {$$=mknode("TRUE",NULL,NULL);} | TOK_FALSE {$$=mknode("FALSE",NULL,NULL);};

/*  הצהרות משתנים  */
var_definition: VAR args ':' type ';' { node* t = $4; t->left = $2; $$ = t; }
              | VAR args ':' TSTRING '[' LIT_INT ']' ';' {
                    char buf[32];
                    sprintf(buf, "STRING[%d]", $6);
                    node* t = mknode(buf, $2, NULL);
                    $$ = t;
              };

/*  פקודות ובלוקים  */
gen_stmts: var_definition gen_stmts { $$ = mknode("", $1, $2); }
         | stmts                    { $$ = $1; }
         |                          { $$ = mknode("NONE", NULL, NULL); }
         ;

stmts: stmt stmts { $$ = mknode("", $1, $2); }
     | stmt       { $$ = $1; }
     ;

stmt: if_stmt | for_stmt | assign_stmt | while_stmt 
    | RETURN expr ';' { $$ = mknode("RET", $2, NULL); }
    | RETURN ';' { $$ = mknode("RET", mknode("NONE", NULL, NULL), NULL); }
    | IDENTIFIER '(' expr_list ')' ';' { $$ = mknode("CALL", mknode($1, NULL, NULL), $3); }
    | '{' gen_stmts '}' { $$ = mknode("BLOCK", $2, NULL); }
    |funcs { $$ = $1; }

if_stmt: IF '(' expr ')' stmt { $$ = mknode("IF", $3, $5); }
       | IF '(' expr ')' stmt ELSE stmt { $$ = mknode("IF-ELSE", $3, mknode("", $5, $7)); };

while_stmt: WHILE '(' expr ')' stmt { $$ = mknode("WHILE", $3, $5); };

for_stmt: FOR '(' inits ';' expr ';' updates ')' stmt { 
            $$ = mknode("FOR", $3, mknode("", $5, mknode("", $7, $9))); 
        };

/*  משפטי השמה  */
assign_stmt: IDENTIFIER ASSIGN expr ';' { $$ = mknode("=", mknode($1, NULL, NULL), $3); }
           | IDENTIFIER '[' expr ']' ASSIGN expr ';' { 
                node* arr = mknode("[]", mknode($1, NULL, NULL), $3);
                $$ = mknode("=", arr, $6); 
             }
           | DEREF expr ASSIGN expr ';' { 
                $$ = mknode("=", mknode("^", $2, NULL), $4); 
             };

expr_list: expr ',' expr_list { $$ = mknode("", $1, $3); }
         | expr               { $$ = $1; }
         |                    { $$ = mknode("NONE", NULL, NULL); }
         ;

/*  ביטויים  */
expr: expr PLUS expr {$$=mknode("+",$1,$3);} | expr MINUS expr {$$=mknode("-",$1,$3);}
    | expr MULT expr {$$=mknode("*",$1,$3);} | expr DIV expr {$$=mknode("/",$1,$3);}
    | expr EQ expr {$$=mknode("==",$1,$3);} | expr NEQ expr {$$=mknode("!=",$1,$3);}
    | expr GT expr {$$=mknode(">",$1,$3);} | expr GE expr {$$=mknode(">=",$1,$3);}
    | expr LT expr {$$=mknode("<",$1,$3);} | expr LE expr {$$=mknode("<=",$1,$3);}
    | expr AND expr {$$=mknode("&&",$1,$3);} | expr OR expr {$$=mknode("||",$1,$3);}
    | NOT expr {$$=mknode("!",$2,NULL);} 
    | MINUS expr %prec UMINUS {$$=mknode("UMINUS",$2,NULL);}
    | PLUS expr %prec UPLUS {$$=$2;} 
    | IDENTIFIER {$$=mknode($1,NULL,NULL);} | type_literals {$$=$1;}
    | IDENTIFIER '[' expr ']' {$$=mknode("[]", mknode($1,NULL,NULL), $3);}
    | '(' expr ')' {$$=$2;} 
    | DEREF expr {$$=mknode("^",$2,NULL);}
    | ADDRESS expr {$$=mknode("&",$2,NULL);}
    | LENGTH IDENTIFIER LENGTH {$$=mknode("|length|", mknode($2,NULL,NULL), NULL);}
    | TOK_NULL {$$=mknode("NULL",NULL,NULL);}
    | IDENTIFIER '(' expr_list ')' {$$=mknode("CALL", mknode($1,NULL,NULL), $3);};

inits: IDENTIFIER ASSIGN expr { $$ = mknode("=", mknode($1,NULL,NULL), $3); }
     | expr                   { $$ = $1; }
     |                        { $$ = NULL; }
     ;
     
updates: IDENTIFIER ASSIGN expr {$$=mknode("=", mknode($1,NULL,NULL), $3);} | {$$=NULL;};

%%

/*  פונקציות העזר וההדפסה  */
#include "lex.yy.c"

void printtree_internal(node* tree, int depth);

int is_block_node(const char* token) {
    if (!token) return 0;
    return (strcmp(token, "PROC") == 0 ||
            strcmp(token, "FUNC") == 0 ||
            strcmp(token, "BODY") == 0 ||
            strcmp(token, "BLOCK") == 0 ||
            strcmp(token, "IF-ELSE") == 0 ||
            strcmp(token, "IF") == 0 ||
            strcmp(token, "WHILE") == 0 ||
            strcmp(token, "FOR") == 0);
}

void print_children(node* n, int depth, int parent_is_block, int* printed_newlines) {
    if (!n) return;
    
    if (n->token == NULL || strcmp(n->token, "") == 0) {
        print_children(n->left, depth, parent_is_block, printed_newlines);
        print_children(n->right, depth, parent_is_block, printed_newlines);
        return;
    }

    int is_leaf = (n->left == NULL && n->right == NULL);

    if (is_leaf && !parent_is_block && !(*printed_newlines)) {
        printf(" %s", n->token);
    } else {
        printf("\n");
        *printed_newlines = 1;
        printtree_internal(n, depth);
    }
}

void printtree_internal(node* tree, int depth) {
    if (!tree) return;
    
    if (tree->token == NULL || strcmp(tree->token, "") == 0) {
        printtree_internal(tree->left, depth);
        if (tree->left && tree->right) printf("\n"); 
        printtree_internal(tree->right, depth);
        return;
    }

    for (int i = 0; i < depth; i++) printf("  ");

    int is_leaf = (tree->left == NULL && tree->right == NULL);
    if (is_leaf) {
        printf("%s", tree->token);
        return;
    }

    printf("(%s", tree->token);

    int parent_is_block = is_block_node(tree->token);
    int printed_newlines = 0;

    print_children(tree->left, depth + 1, parent_is_block, &printed_newlines);
    print_children(tree->right, depth + 1, parent_is_block, &printed_newlines);

    if (printed_newlines) {
        printf("\n");
        for (int i = 0; i < depth; i++) printf("  ");
    }
    printf(")");
}

void printtree(node *tree, int depth) {
    printtree_internal(tree, depth);
}

int main() {
    if (yyparse() == 0) {
        printf("(CODE\n");
        printtree(root, 1);
        printf("\n)\n");
    }
    return 0;
}

int yyerror(const char* s) { 
    printf("Error at line %d: %s (at token '%s')\n", line_number, s, yytext); 
    return 0; 
}
node *mknode(char* token, node* left, node* right) {
    node *n = (node*)malloc(sizeof(node));
    n->token = token ? strdup(token) : NULL;
    n->left = left; n->right = right;
    return n;
}